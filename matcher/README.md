# Yojimbo Matcher Sample

This is a sample matcher server written in go that will provide a connection token via the following endpoint:

```
GET /match/{protocolID}/{clientID}
```

# Building the Docker image:

To build the image run the following command from the `matcher` directory:

```sh
docker build --tag=matcher .
```

The builder's Go version is the `GO_VERSION` build argument, defaulting to the version
`go.mod` requires. CI builds this image on every change and fails if the two disagree, so the
documented command is a command that works.

# Running the Docker container:

Run the container image mapping the port to your host machine:

```sh
docker run -d -p 8081:8081 --name matcher matcher
```

# Using the matcher:

To hit the container with a test request:

```sh
PROTOCOL_ID=123 && CLIENT_ID=42 && curl http://localhost:8081/match/${PROTOCOL_ID}/${CLIENT_ID}
```
