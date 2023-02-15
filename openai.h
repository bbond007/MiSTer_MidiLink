#include <curl/curl.h>
#include <stdbool.h>

#define ESC_KEY      0x1b
#define RTN_KEY      0x0a
#define DEL_KEY      0x7f
#define BSL_KEY      0x5c

CURL * openai_init();
CURLcode openai_say(char * msg);
void openai_done();
void openai_reset_chunk();
void openai_parse_json_content(char * jsonContent, size_t jsonContentSize, bool reset);
void openai_parse_jsmn_content(char * jsonContent, size_t jsonContentSize);

struct openai_memory_chunk {
  char *response;
  size_t size;
};
