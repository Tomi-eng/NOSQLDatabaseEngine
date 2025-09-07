#include "protocol.h"


// Encode response to bytes
/* 
[4-byte length]
[1-byte ResponseStatus]   OK = 1, NOT_FOUND = 2, ERROR = 3,
[2-byte ItemCount]     ; how many items follow
   repeat ItemCount times:
       [2-byte ValLen] ; for SET, 0 for DEL/GET
       [Value bytes]   ; only if SET
*/
std::vector<u_int8_t> EncodeResponse(const Response &res){
    uint16_t nitems, KeyLen, ValLen;
    uint32_t length;
    std::vector<u_int8_t> buffer;
    std::string s;

    //Reserve 4 bytes for response length
    buffer.resize(4);

    //Append response status
    buffer.push_back(static_cast<u_int8_t>(res.status));

    //Append ItemCount 
    nitems = res.values.size();
    buffer.push_back(nitems & 0xFF);
    buffer.push_back((nitems >> 8) & 0xFF);


    for(int i =0; i < res.values.size(); i ++){
        s = res.values[i];
        ValLen = s.size();
        buffer.push_back(ValLen & 0xFF);
        buffer.push_back((ValLen >> 8) & 0xFF);
        buffer.insert(buffer.end(), s.begin(), s.end());
    }
    
    // Add length of request at start of buffer 
    length = buffer.size() - 4;
    buffer[0] = length & 0xFF;
    buffer[1] = (length >> 8) & 0xFF;
    buffer[2] = (length >> 16) & 0xFF;
    buffer[3] = (length >> 24) & 0xFF;
    return buffer;
}


/* 
[1-byte OpCode]        ; 1 = GET, 2 = SET, 3 = DEL
[2-byte ItemCount]     ; how many items follow
   repeat ItemCount times:
       [2-byte KeyLen]
       [2-byte ValLen] ; for SET, 0 for DEL/GET
       [Key bytes]
       [Value bytes]   ; only if SET
*/
Request DecodeRequest(std::vector<u_int8_t> &data){
    size_t offset = 0;
    struct Request req;
    uint16_t nitems, KeyLen, ValLen;
    struct CommandArgument cmdargs;
    req.cmd = static_cast<Command>(data[offset]);
    offset++;

    // get number of items in request vector 
    nitems = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    for(int i = 0; i < nitems;  i++){
        KeyLen = data[offset] | (data[offset + 1] << 8);
        offset += 2;
        ValLen = data[offset] | (data[offset + 1] << 8); 
        offset += 2;

        std::string key(reinterpret_cast<const char*>(data[offset]), KeyLen);
        cmdargs.key = key;
        offset += KeyLen;

        std::string val(reinterpret_cast<const char*>(data[offset]), ValLen);
        cmdargs.value = val;
        req.args.push_back(cmdargs);
        offset += ValLen;
    }
    return req;
}

// Encode request to bytes
/* 
[4-byte length]
[1-byte OpCode]        ; 1 = GET, 2 = SET, 3 = DEL
[2-byte ItemCount]     ; how many items follow
   repeat ItemCount times:
       [2-byte KeyLen]
       [2-byte ValLen] ; for SET, 0 for DEL/GET
       [Key bytes]
       [Value bytes]   ; only if SET
*/
std::vector<u_int8_t> EncodeRequest(const Request &query){
    uint16_t nitems, KeyLen, ValLen;
    uint32_t length;
    std::vector<u_int8_t> buffer;
    struct CommandArgument args;
    std::string s;

    //Reserve 4 bytes for response length
    buffer.resize(4);

    //Append command
    buffer.push_back(static_cast<u_int8_t>(query.cmd));

    //Append ItemCount 
    nitems = query.args.size();
    buffer.push_back(nitems & 0xFF);
    buffer.push_back((nitems >> 8) & 0xFF);


    for(int i = 0; i < query.args.size(); i++){
        args = query.args[i];
        KeyLen = args.key.size();
        buffer.push_back(KeyLen & 0xFF);
        buffer.push_back((KeyLen >> 8) & 0xFF);
        ValLen = args.value.size();
        buffer.push_back(ValLen & 0xFF);
        buffer.push_back((ValLen >> 8) & 0xFF);
        buffer.insert(buffer.end(), args.key.begin(), args.key.end());
        buffer.insert(buffer.end(), args.value.begin(), args.value.end());
    }
    
    // Add length of request at start of buffer 
    length = buffer.size() - 4;
    buffer[0] = length & 0xFF;
    buffer[1] = (length >> 8) & 0xFF;
    buffer[2] = (length >> 16) & 0xFF;
    buffer[3] = (length >> 24) & 0xFF;
    return buffer;
}

/* 
[1-byte ResponseStatus]   OK = 1, NOT_FOUND = 2, ERROR = 3,
[2-byte ItemCount]     ; how many items follow
   repeat ItemCount times:
       [2-byte ValLen]
       [Value bytes]
*/
Response DecodeResponse(std::vector<u_int8_t> &data){
    size_t offset = 0;
    struct Response res;
    uint16_t nitems, ValLen;

    res.status = static_cast<ResponseStatus>(data[offset]);
    offset++;

    // get number of items in request vector 
    nitems = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    for(int i = 0; i < nitems; i++){
        ValLen = data[offset] | (data[offset + 1] << 8); 
        offset += 2;
        std::string val(reinterpret_cast<const char*>(data[offset]), ValLen);
        res.values.push_back(val);
        offset += ValLen;
    }
    return res;
}