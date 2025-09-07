
#include <cstdint>
#include <string>
#include <vector>

// protocol 
//Example: GET key1 key2 ...Keyn
//Example: DEL key1 key2 ...Keyn
//SET key1 value1
/*[4-byte Length]
[1-byte OpCode]        ; 1 = GET, 2 = SET, 3 = DEL
[2-byte ItemCount]     ; how many items follow
   repeat ItemCount times:
       [2-byte KeyLen]
       [2-byte ValLen] ; for SET, 0 for DEL/GET
       [Key bytes]
       [Value bytes]   ; only if SET
*/
enum class Command: uint8_t {
    GET = 1,
    SET = 2,
    DEL = 3
};

enum class ResponseStatus: uint8_t {
    OK = 1,
    NOT_FOUND = 2,
    ERROR = 3,
};

struct CommandArgument {
    std::string key;
    std::string value;  // empty if GET/DEL
};


struct Request {
    Command cmd;
    std::vector<CommandArgument> args;
};

struct Response {
    ResponseStatus status;
    std::vector<std::string> values; // only for GET
};


std::vector<u_int8_t> EncodeResponse(const Response &res);
Request DecodeRequest(std::vector<u_int8_t> &data);
std::vector<u_int8_t> EncodeRequest(const Request &query);
Response DecodeResponse(std::vector<u_int8_t> &data);
