//
// Created by gogo on 6/25/25.
//
#include "httpImp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * handle_get(char *path) {
    char *response = malloc(MAXDATASIZE);
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s","{\"id\": 1}");
    return response;
}

char * handle_post(char * request, char* path) {

    char *body = strstr(request, "\r\n\r\n");
    char *handler_result = NULL;
    char *response = NULL;

    if (strcmp(path, "/process") == 0) {
        handler_result = handle_process(body);
    }
    
    else {
        return strdup("HTTP/1.1 404 Not Found\r\n\r\n");
    }

    if (handler_result) {

        int needed_size = snprintf(NULL, 0, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", handler_result);
        response = malloc(needed_size + 1);
        if (response == NULL) {
            perror("Failed to allocate memory!!!");
        }
        if (response) {

            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", handler_result);
        }
    }

    if (!response) {
        response = strdup("HTTP/1.1 500 Internal Server Error\r\n\r\n");
    }
    return response;
}



static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};
void build_decoding_table() {

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void base64_cleanup() {
    free(decoding_table);
}

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}



char * handle_process(char * request) {

    json_object * jdata = json_tokener_parse(request);
    char * out_program;
    char *out_data;
    extract_js_packet(jdata, "program", &out_program);
    extract_js_packet(jdata, "data", &out_data);

    FILE *f_program;

    f_program = fopen("out_program.out", "wb");

    if (f_program == NULL) {
        json_object_put(jdata);
        free(out_program);
        perror("Failed to open file for writing!!!");
        return "Internal server error!!!\n";
    }

    size_t n_out_program = strlen(out_program);

    // Allocate memory for the decoded binary data
    // Decoded size is roughly 3/4 of the encoded size
    size_t max_decoded_size = (n_out_program * 3) / 4 + 1;
    unsigned char *binary_data = malloc(max_decoded_size);
    if (binary_data == NULL) {
        perror("Failed to allocate memory");
        return "Internal server error!!!\n";
    }

    // Decode the Base64 string
    size_t decoded_size;
    binary_data = base64_decode(out_program, n_out_program, &decoded_size);

    fwrite(binary_data, 1, decoded_size, f_program);
    fclose(f_program);
    free(out_program);
    free(binary_data);


    FILE *f_data;

    f_data = fopen("out_data.csv", "w");

    if (f_data == NULL) {
        json_object_put(jdata);
        perror("Failed to open file for writing!!!");
        return "Internal server error!!!\n";
    }

    fputs(out_data, f_data);
    fclose(f_program);

    system("chmod 777 out_program.out");
    int result = system("./out_program.out out_data.csv");

    if (result == 0) {
        FILE *f_data_result;
        f_data_result = fopen("output.csv", "r");

        if (f_data_result == NULL) {
            perror("Failed to open file output.csv");
            return "Internal server error!!!\n";
        }

        char *data_result = NULL;
        int ch;
        int i = 0;

        while ((ch = fgetc(f_data_result)) != EOF) {
            data_result = realloc(data_result, (i + 2) * sizeof(char)); // Make room for new char + '\0'
            if (data_result == NULL) {
                fclose(f_data_result);
                return "Internal server error!!!\n";
            }
            data_result[i] = (char)ch; // Store the character
            i++;
        }
        if (data_result) {
            data_result[i] = '\0'; // Null-terminate
        }

        fclose(f_data_result);

        json_object * j_result = json_object_new_object();;
        char *out;
        create_js_packet_procees_result_server(j_result,data_result, &out);
        free(data_result);
        json_object_put(j_result);
        return out;
    }

    return "Internal server error!!!\n";
}

char * procces_request(char * request) {
    char method[10], path[50];
    sscanf(request, "%s %s", method, path);
    if (strcmp(method, "GET") == 0) {
        return handle_get(path);
    }

    if (strcmp(method, "POST") == 0) {
        return handle_post(request, path);
    }
    return "Internal server error!!!\n";
}

char * handle_exit() {
    return "{\"exit\": true}'\n";
}