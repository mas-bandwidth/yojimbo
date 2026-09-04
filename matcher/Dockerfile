# The Go version is one value, defaulted here and checked against go.mod in CI (YJ-07). This
# image used to pin golang:1.20.13 while go.mod required go 1.25.0, so the documented build
# could not succeed -- and nothing built the image, so nobody found out.
ARG GO_VERSION=1.25.0

FROM golang:${GO_VERSION} AS matcher_build

# Matcher
WORKDIR /matcher

# Copy go.mod and go.sum files to the workspace separately and download dependecies.
# Doing this separately will cache these as its own separate layer
COPY ./go.mod .
COPY ./go.sum .
RUN go mod download

# Copy the source code as the last step
COPY . .

RUN go vet ./...
RUN go test ./...

# Build the binary
RUN CGO_ENABLED=0 go build -o matcher.bin main.go

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Then we copy and run it from a slim image. CGO_ENABLED=0 above makes the binary static, so
# the runtime carries nothing else.
FROM alpine:3.22
WORKDIR /matcher

COPY --from=matcher_build /matcher/matcher.bin .

EXPOSE 8081

ENTRYPOINT ["/matcher/matcher.bin"]
