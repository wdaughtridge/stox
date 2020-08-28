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

void concatArgs(int numArgs, const char *argVals[], char *url) {
	strcpy(url, "http://api.marketstack.com/v1/eod"
								 "?access_key=390e2a1f207b6534df8019e00a2f18c7");

	for (int i = 1; i < numArgs; i++) {
		if (!strncmp("--symbols=", argVals[i], 10)) {
			strcat(url, "&");
			strcat(url, argVals[i] + 2);
		} else if (!strncmp("--date_from=", argVals[i], 12)) {
			strcat(url, "&");
			strcat(url, argVals[i] + 2);
		} else if (!strncmp("--date_to=", argVals[i], 10)) {
			strcat(url, "&");
			strcat(url, argVals[i] + 2);
		} else if (!strncmp("--sort=", argVals[i], 7)) {
			strcat(url, "&");
			strcat(url, argVals[i] + 2);
		} else {
			printf("Unkown command line arg!: %s", argVals[i]);
		}
	}
}

int exportToFile(cJSON *data) {
	char *prettyData = cJSON_Print(data);

	if (!prettyData) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
				fprintf(stderr, "Error before: %s\n", error_ptr);
		}
		return -1;
	}

	FILE *jsonResults = fopen("stats.json", "w+");

	if (!jsonResults) {
		fclose(jsonResults);
		return -1;
	}

	fputs(prettyData, jsonResults);
	fclose(jsonResults);

	return 0;
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

    char requrl[1023];
		concatArgs(argc, argv, requrl);

    curl_easy_setopt(curl, CURLOPT_URL, requrl);

		// perform request for data
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "REQUEST FAILED! Error: %s\n", curl_easy_strerror(res));
      goto END;
    }

		// parse out request data with cJSON
    cJSON *results = cJSON_Parse(chunk.response);
    if (!results) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL) {
          fprintf(stderr, "cJSON ERROR: %s\n", error_ptr);
      }
      goto END;
    }

		// get data object where all the actual info is
		const cJSON *stockData = cJSON_GetObjectItemCaseSensitive(results, "data");
		const cJSON *oneDayData;

		cJSON_ArrayForEach(oneDayData, stockData) {
			cJSON *open 			= cJSON_GetObjectItemCaseSensitive(oneDayData, "open");
			cJSON *high 			= cJSON_GetObjectItemCaseSensitive(oneDayData, "high");
			cJSON *low  			= cJSON_GetObjectItemCaseSensitive(oneDayData, "low");
			cJSON *close 			= cJSON_GetObjectItemCaseSensitive(oneDayData, "close");
			cJSON *volume			= cJSON_GetObjectItemCaseSensitive(oneDayData, "volume");
			cJSON *adj_high 	= cJSON_GetObjectItemCaseSensitive(oneDayData, "adj_high");
			cJSON *adj_low 		= cJSON_GetObjectItemCaseSensitive(oneDayData, "adj_low");
			cJSON *adj_close 	= cJSON_GetObjectItemCaseSensitive(oneDayData, "adj_close");
			cJSON *adj_open 	= cJSON_GetObjectItemCaseSensitive(oneDayData, "adj_open");
			cJSON *adj_volume = cJSON_GetObjectItemCaseSensitive(oneDayData, "adj_volume");
			cJSON *symbol 		= cJSON_GetObjectItemCaseSensitive(oneDayData, "symbol");
			cJSON *exchange 	= cJSON_GetObjectItemCaseSensitive(oneDayData, "exchange");
			cJSON *date 			= cJSON_GetObjectItemCaseSensitive(oneDayData, "date");
		}

		if (exportToFile(results))
			printf("Writing to file failed!\n");

END:
    curl_easy_cleanup(curl);
  }
}
