#include "cJSON/cJSON.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

struct memory {
  char *response;
  size_t size;
};

static size_t cb(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if (ptr == NULL)
    return 0;

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

int main(int argc, const char *argv[]) {
  if (argc == 1) {
    printf("Not enough arguments!\n");
    exit(EXIT_FAILURE);
  }

  CURL *curl;
  CURLcode res = CURLE_OK;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (curl) {
    struct memory chunk;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // TODO: check to see if cmd line args will not over fill buffer
    char requrl[1023];
    strcpy(requrl, "http://api.marketstack.com/v1/eod"
                   "?access_key=390e2a1f207b6534df8019e00a2f18c7");

    for (int i = 1; i < argc; i++) {
      if (!strncmp("--symbols=", argv[i], 10)) {
        strcat(requrl, "&");
        strcat(requrl, argv[i] + 2);
      } else if (!strncmp("--date_from=", argv[i], 12)) {
        strcat(requrl, "&");
        strcat(requrl, argv[i] + 2);
      } else if (!strncmp("--date_to=", argv[i], 12)) {
        strcat(requrl, "&");
        strcat(requrl, argv[i] + 2);
      } else if (!strncmp("--sort=", argv[i], 12)) {
        strcat(requrl, "&");
        strcat(requrl, argv[i] + 2);
      }
    }

    curl_easy_setopt(curl, CURLOPT_URL, requrl);
    res = curl_easy_perform(curl);

    cJSON *results = cJSON_Parse(chunk.response);
    if (!results)
      return -1;

    char *prettyResults = cJSON_Print(results);
    if (!prettyResults)
      return -1;

    FILE *jsonResults = fopen("stats.json", "w+");

    if (jsonResults)
      fputs(prettyResults, jsonResults);

    fclose(jsonResults);

    if (res != CURLE_OK)
      fprintf(stderr, "REQUEST FAILED! Error: %s\n", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
  }
}
