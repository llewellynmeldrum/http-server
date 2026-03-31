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
typedef struct ByteStream{

}

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

