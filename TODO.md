# TODO

Server addresses array in matcher should be passed in as raw strings and then base64'd inside GenerateConnectToken

Now work out how to integrate with libsodium in go

Implement "GenerateKey" function

Encrypt the connect token with the private key via libsodium.

Base64 this token.

Update web app to generate a response with JSON for data, with a base64 encoding of the encrypted connect token,
and then JSON fields for all the other bits of data the client needs to have returned to them.

Now work out how to get the client passing in the parameters of what they want for the match, eg. their clientId
and the protocolId they are looking for.

Update the web response to be on a different URI, eg. /match

What to return if no match can be found? JSON with no servers to connect to, failure error code? no connect token?

