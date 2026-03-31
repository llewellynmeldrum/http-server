# Thinking about api design

# API Redesign:
1 The IncomingConnection produces the incoming_data byte array.
2 The RequestParser parses the raw incoming_data, producing a HttpRequest
3 The RequestResolver consumes a ParsedRequest, producing a ResourceResult
4 The ResponseGenerator consumes a ResourceResult, producing a HttpResponse
5 The ResponseEncodr consumes a HttpResponse, producing the outgoing_data byte_array.

# Table:
Stage                   Consumes            Produces
Incoming Connection     ...                 incoming_data
RequestParser           incoming_data       HttpRequest
RequestResolver         HttpRequest         Resource
ResponseGenerator       Resource            HttpResponse
ResponseEncoder         HttpResponse        outgoing_data
OutgoingConnection      outgoing_data       ...


```C
// 
typedef uint8_t Byte;
constexpr size_t BYTE_BUFFER_CAP = 1024;
typedef struct {
    Byte data[BYTE_BUFFER_CAP];
    size_t size;
}ByteBuffer;

typedef struct {
    
}ByteBuffer;





```

## Consumes vs produces
ClientSends()
    ...              ->   IncomingConnection()    -> ByteArray incoming_data
    incoming_data    ->        RequestParser()    -> Request request
    request          ->      RequestResolver()    -> ResourceResult res 
    resolutionResult ->    ResponseGenerator()    -> Response response
    response         ->      ResponseEncoder()    -> ByteArray outgoing_data
    outgoing_data    ->   ...
ClientRecieves()

# Table:
Stage                   Consumes            Produces
Incoming Connection     ...                 incoming_data
RequestParser           incoming_data       HttpRequest
RequestResolver         HttpRequest         Resource
ResponseGenerator       Resource            HttpResponse
ResponseEncoder         HttpResponse        outgoing_data
OutgoingConnection      outgoing_data       ...

```C
HttpRequest ParseRequest(Byte [BUF_SZ] incoming_data);
// in  main.c  
incoming_da
Byte incoming_data[BUF_SZ] = {};
size_t bytes_received = recv(client_fd, incoming_data, arrlen(incoming_data), 0);

HttpRequest http_request = parseRequest(incoming_data);
Resource resource = resolveRequest(http_request);
HttpResponse response = generateResponse(resource);
Byte* outgoing_data = encodeResponse(response);
```

