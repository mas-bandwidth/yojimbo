This guide is made to help you getting started with Yojimbo, integrate it into your game and cover some of the basic usages of the library.

Before using any other part of Yojimbo, you must call `InitializeYojimbo()` once at startup, and `ShutdownYojimbo()` once when you are done. This applies to both the client and the server. `InitializeYojimbo()` returns `false` if the library failed to initialize, and `ShutdownYojimbo()` runs some checks for you, such as memory-leak detection in debug builds:

```cpp
int main() {
    if (!InitializeYojimbo()) {
        // handle the error
        return 1;
    }

    // ... run your client or server ...

    ShutdownYojimbo();
    return 0;
}
```

Yojimbo is meant to be used in a client/server architecture. The first thing you will need is a way to run your game "headless" (with no window / rendering context) for the server. Game servers usually run in VPS machines where there is no real use for a window or rendering to the screen. Let's create our main server class called `GameServer` and let it handle everything the server needs to do. The different parts of Yojimbo you need on the server are `Server`, `ClientServerConfig` and `Adapter`.

The `Server` is the main part, used to receive and send messages. The `ClientServerConfig` allows you to configure the server: connection timeout, memory used or the different channels (reliable, unreliable). The `Adapter` allows you to get callbacks when a client connects or disconnects and is also responsible for providing Yojimbo with the `MessageFactory`. Both the `ClientServerConfig` and the `Adapter` are shared between client and server. Here is what it could look like:

```cpp
// a simple test message
enum class GameMessageType {
    TEST,
    COUNT
};

// two channels, one for each type that Yojimbo supports
enum class GameChannel {
    RELIABLE,
    UNRELIABLE,
    COUNT
};

// the client and server config
struct GameConnectionConfig : yojimbo::ClientServerConfig {
    GameConnectionConfig()  {
        numChannels = 2;
        channel[(int)GameChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int)GameChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

// the adapter
class GameAdapter : public yojimbo::Adapter {
public:
    explicit GameAdapter(GameServer* server = NULL) :
        m_server(server) {}

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }

    void OnServerClientConnected(int clientIndex) override {
        if (m_server != NULL) {
            m_server->ClientConnected(clientIndex);
        }
    }

    void OnServerClientDisconnected(int clientIndex) override {
        if (m_server != NULL) {
            m_server->ClientDisconnected(clientIndex);
        }
    }

private:
    GameServer* m_server;
};

// the message factory
YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int)GameMessageType::COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::TEST, TestMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();
```

Note that the adapter will have a null `GameServer` pointer when used on the client. If you prefer, you can also create a different adapter for the client, but it needs to provide Yojimbo with the same `MessageFactory` as the server.

A note on configuration: the config must be **identical on the client and the server**, or they won't be able to communicate. In debug builds, the config is validated automatically when the server starts and when the client connects — an invalid value asserts with an error naming the field and the required value. One thing to watch out for: `maxPacketFragments` is derived from `maxPacketSize` inside the `ClientServerConfig` constructor, so if you increase `maxPacketSize` in your own config, update `maxPacketFragments` to match (the validation catches this if you forget). In release builds validation compiles away entirely — correct configuration is your responsibility.

Let's take a look at the `TestMessage` class and briefly cover basic serialization:

```cpp
class TestMessage : public yojimbo::Message {
public:
    int m_data;

    TestMessage() :
        m_data(0) {}

    template <typename Stream>
    bool Serialize(Stream& stream) {
        serialize_int(stream, m_data, 0, 512);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};
```

Yojimbo uses a unified serialization system for both reading from and writing to a `Stream`. It might take a bit of getting used to, and for more complex messages, you will sometimes have to split the code path, but we won't get into details here. For now we just want to read/write a simple int, so we use the `serialize_int` helper. There are several serialization helpers provided by Yojimbo and you will probably need to define several more to cover the needs of your game.

The `GameServer` will then have a `GameConnectionConfig`, a `GameAdapter` and a `Server` and initialize everything like so:

```cpp
static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = { 0 };
static const int MAX_PLAYERS = 64;

GameServer::GameServer(const yojimbo::Address& address) :
    m_adapter(this),
    m_server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, address, m_connectionConfig, m_adapter, 0.0)
{
    // start the server
    m_server.Start(MAX_PLAYERS);
    if (!m_server.IsRunning()) {
        throw std::runtime_error("Could not start server at port " + std::to_string(address.GetPort()));
    }

    // print the port we got in case we used port 0
    char buffer[256];
    m_server.GetAddress().ToString(buffer, sizeof(buffer));
    std::cout << "Server address is " << buffer << std::endl;

    // ... load game ...
}

void GameServer::ClientConnected(int clientIndex) {
    std::cout << "client " << clientIndex << " connected" << std::endl;
}

void GameServer::ClientDisconnected(int clientIndex) {
    int reason = m_server.GetClientDisconnectReason(clientIndex);
    std::cout << "client " << clientIndex << " disconnected: "
              << yojimbo::GetServerClientDisconnectReasonString(reason) << std::endl;
}
```

The server tracks *why* the client in each slot was last disconnected, and the reason is recorded before `OnServerClientDisconnected` is called, so you can query it from inside the callback like above. `Server::GetClientDisconnectReason` returns a `ServerClientDisconnectReason` value (see `yojimbo_server.h`): a client that quit cleanly reports `YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DISCONNECTED`, a client you stopped hearing from reports `..._TIMED_OUT`, a client you kicked with `DisconnectClient` reports `..._KICKED`, and connection errors report the specific cause — `..._FAILED_TO_SERIALIZE` usually means a client running a different protocol version, and `..._OUT_OF_MEMORY` means `serverPerClientMemory` is undersized for real traffic. Routing these into your metrics is how you find out whether disconnects in the wild are your bug, your config, or just the network.

Note the use of a null private key. How to make secure connections to the server is a topic on its own that won't be covered here. But for development, you will be running both the server and the client on your own machine so you're fine with insecure connections.

Also note that the `GameServer` takes an `Address`. The IP part of the address must be set to the external IP address of the machine. You are free to choose the port. For development, just use "127.0.0.1" and a port of your liking. For several LAN machines connecting together, use the local IP address of the server.

The server will need a custom game loop, running at a fixed timestep. Game loops and whether to use fixed timestep or not is also a topic on its own, but for netcode, fixed timestep, for both server and client, is often a good idea. We chose to run the server at 60Hz. Here is what it looks like:

```cpp
void GameServer::Run() {
    float fixedDt = 1.0f / 60.0f;
    while (m_running) {
        double currentTime = yojimbo_time();
        if (m_time <= currentTime) {
            Update(fixedDt);
            m_time += fixedDt;
        } else {
            yojimbo_sleep(m_time - currentTime);
        }
    }
}
```

In the `Update` method, there are 3 methods of the `Server` class that you need to call: `AdvanceTime`, `ReceivePackets` and `SendPackets`. Note that the order is very important. Using another order might add a couple frames of delay to your server's latency. Here is what it looks like:

```cpp
void GameServer::Update(float dt) {
    // stop if server is not running
    if (!m_server.IsRunning()) {
        m_running = false;
        return;
    }

    // update server and process messages
    m_server.AdvanceTime(m_time);
    m_server.ReceivePackets();
    ProcessMessages();

    // ... process client inputs ...
    // ... update game ...
    // ... send game state to clients ...

    m_server.SendPackets();
}
```

The `ProcessMessage` function consists in looping over all the connected clients and read the messages received from each of them:

```cpp
void GameServer::ProcessMessages() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (m_server.IsClientConnected(i)) {
            for (int j = 0; j < m_connectionConfig.numChannels; j++) {
                yojimbo::Message* message = m_server.ReceiveMessage(i, j);
                while (message != NULL) {
                    ProcessMessage(i, message);
                    m_server.ReleaseMessage(i, message);
                    message = m_server.ReceiveMessage(i, j);
                }
            }
        }
    }
}

void GameServer::ProcessMessage(int clientIndex, yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::TEST:
        ProcessTestMessage(clientIndex, (TestMessage*)message);
        break;
    default:
        break;
    }
}

void GameServer::ProcessTestMessage(int clientIndex, TestMessage* message) {
    // ... process test message ...
}
```

Let's leave the server aside for a moment and take a look at the client. You need a `Client` and the same `ClientServerConfig` and `Adapter` as the server. In this example, the game is divided into "screens" and the screen handling the online game is called `OnlineGameScreen`. It looks like this:

```cpp
OnlineGameScreen::OnlineGameScreen(const yojimbo::Address& serverAddress) :
    m_client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), m_connectionConfig, m_adapter, 0.0)
{
    uint64_t clientId;
    yojimbo_random_bytes((uint8_t*)&clientId, 8);
    m_client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, m_serverAddress);
}
```

Yojimbo requires each client to have a unique `clientId`. In a game with user accounts, this would typically be the user id. While in development or if you don't have user accounts in your game, you can just pass a random `uint64_t` number. Note that with a secure connection, the `clientId` wouldn't be set by the client, but would come inside a connection token received from the web backend, so clients cannot spoof their identity. But this is out of the scope of this guide.

The client is now connecting. You can check the state of the connection in your game loop to know when the connection is established. Note that later on, you will also want to detect disconnections and try to reconnect the client. Just like on the server, you also need to update the `Client` by calling the `AdvanceTime`, `ReceivePackets` and `SendPackets` methods.

When the client ends up disconnected, ask it why with `Client::GetDisconnectReason`. It returns a `ClientDisconnectReason` value (see `yojimbo_client.h`) that tells you what to show the player and what to do next:

```cpp
if (m_client.IsDisconnected() || m_client.ConnectionFailed()) {
    switch (m_client.GetDisconnectReason()) {
    case YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_DENIED:
        // the server refused us, eg. it is full -> try another server
        break;
    case YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECT_TOKEN_EXPIRED:
        // ask the matchmaker for a fresh connect token and retry
        break;
    case YOJIMBO_CLIENT_DISCONNECT_REASON_FAILED_TO_SERIALIZE:
        // protocol mismatch -> tell the player to update their client
        break;
    case YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_TIMED_OUT:
        // network problem -> offer to reconnect
        break;
    // ... and so on. GetClientDisconnectReasonString(reason) gives you a log-friendly string
    }
}
```

The reason is cleared back to `YOJIMBO_CLIENT_DISCONNECT_REASON_NONE` whenever a new connect attempt starts. Connecting to a server that isn't running reports `YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_REQUEST_TIMED_OUT` after the connection timeout, for both secure and insecure connects. `YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECT_TOKEN_EXPIRED` means the connect token's expiry passed before the client could connect — for example, the player sat on a menu too long after matchmaking — and the fix is to request a fresh token and retry.

For testing purposes, let's also send a `TestMessage` when the player presses a key:

```cpp
void OnlineGameScreen::Update(float dt) {
    // update client
    m_client.AdvanceTime(m_client.GetTime() + dt);
    m_client.ReceivePackets();

    if (m_client.IsConnected()) {
        ProcessMessages();

        // ... do connected stuff ...

        // send a message when space is pressed
        if (KeyIsDown(Key::SPACE)) {
            TestMessage* message = (TestMessage*)m_client.CreateMessage((int)GameMessageType::TEST);
            message->m_data = 42;
            m_client.SendMessage((int)GameChannel::RELIABLE, message);
        }
    }

    m_client.SendPackets();
}

void OnlineGameScreen::ProcessMessages() {
    for (int i = 0; i < m_connectionConfig.numChannels; i++) {
        yojimbo::Message* message = m_client.ReceiveMessage(i);
        while (message != NULL) {
            ProcessMessage(message);
            m_client.ReleaseMessage(message);
            message = m_client.ReceiveMessage(i);
        }
    }
}

void OnlineGameScreen::ProcessMessage(yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::TEST:
        ProcessTestMessage((TestMessage*)message);
        break;
    default:
        break;
    }
}

void OnlineGameScreen::ProcessTestMessage(TestMessage* message) {
    std::cout << "Test message received from server with data " << message->m_data << std::endl;
}
```

The client now sends a `TestMessage` when a key is pressed and will also log to the console when receiving one. Let's change the server so that when it receives a `TestMessage` it will log to the console and answer the same message back to the client:

```cpp
void GameServer::ProcessTestMessage(int clientIndex, TestMessage* message) {
    std::cout << "Test message received from client " << clientIndex << " with data " << message->m_data << std::endl;
    TestMessage* testMessage = (TestMessage*)m_server.CreateMessage(clientIndex, (int)GameMessageType::TEST);
    testMessage->m_data = message->m_data;
    m_server.SendMessage(clientIndex, (int)GameChannel::RELIABLE, testMessage);
}
```

Note that we used the `RELIABLE` channel here. We won't get into details but you will want to use the reliable channel for important and unfrequent messages such as initialization or chat messages and the unreliable channel for messages sent every frame like the game state.

You should now be able to run the server, run the client, have it connect to the server and send and receive test messages. Hopefully this guide gave you a better idea on how to get started using Yojimbo!
