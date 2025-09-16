#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "http_response.h"
#include "http_consts.h"
#include "log.h"
#define MAX_FILE_SIZE_B (ssize_t)(1e7)

static char *get_mime_type(char* filename);


// windows/network style
#define NEWL "\r\n"
// macos/linux style
//#define NEWL "\n"

// *INDENT-OFF*
// caller responsible for freeing buffer
char *serialize_http_response(http_response_cfg c) {
	char *buf = malloc(sizeof(char) * BUF_SZ);
	sprintf(buf,
	        "HTTP/%s %d %s"NEWL\
	 	"Content-Type: %s"NEWL\
	 	NEWL\
		"%s"NEWL,
	        c.version, c.status, c.reason_phrase,
	        c.mime_type,
	        c.msg_body
	       );

	if (c.malloced_body) free(c.msg_body);

	return buf;
}

ssize_t syscall_file_size(const char* filename){
	struct stat s;
	stat(filename, &s);
	return s.st_size;
}

#define logretnull(fmt, ...)do{\
	logfatal(fmt, ##__VA_ARGS__)\
	return NULL;\
	}while(0)

char* read_file(const char* filename){
	FILE * file_ptr = fopen(filename, "r");

	if (!file_ptr){
		logfatal("Unable to open file '%s'.\n",filename);
		return NULL;
	}

	ssize_t file_size = syscall_file_size(filename);
	if (file_size==0){
		logfatal("syscall_file_size(%s) ret=0.\n", filename); 
		fclose(file_ptr);
		return NULL;
	}

	if (file_size>MAX_FILE_SIZE_B){
		logfatal("Requested file '%s' is too large! (%zu>%zu)\n", filename, file_size, MAX_FILE_SIZE_B);
		fclose(file_ptr);
		return NULL;
	}
	char* file_contents = malloc(sizeof(char) * file_size);
	if (!file_contents){
		logfatal("Unable to alloc buffer for file '%s'.\n",filename);
		fclose(file_ptr);
		return NULL;
	}

	int n_read = fread(file_contents, file_size, 1, file_ptr); 
	if (n_read!=1){
		logfatal("Unable to read file contents for file '%s'.\n",filename);
		free(file_contents);
		fclose(file_ptr);
		return NULL;
	}
	fclose(file_ptr);
	return file_contents;
}

char *build_http_response(char* request_filename) {
	http_response_cfg cfg;
	if (strncmp(request_filename, "\\",1)){
		// default request, serve index.html 
		request_filename = "index.html";
	}
	log("building response for file --> '%s'\n",request_filename);

	/* 2. Check if file exists*/
	bool file_exists = access(request_filename, F_OK) == 0;
	char* file_contents = read_file(request_filename);
	if (!file_exists || !file_contents) {
		cfg = HTTP_ERR_NOT_FOUND;
		return serialize_http_response(cfg);
	}

	char *mime_type = get_mime_type(request_filename);
	if (!mime_type) {
		cfg = HTTP_ERR_BAD_REQUEST;
		return serialize_http_response(cfg);
	} else {
		cfg = (http_response_cfg) {
			.version = HTTP_VERSION,
			.status = 200,
			.reason_phrase = "200: OK",
			.mime_type = mime_type,
			.msg_body = file_contents,
			.malloced_body = true,
		};
		return serialize_http_response(cfg);
	}
	return NULL;
}

char *get_file_extension(char* filename) {
	char *first_dot =  strchr(filename, '.');
	if (strchr(first_dot + 1, '.')) {
		return NULL;
	} else {
		return first_dot;
	}
}
#define streq(s1, s2) (strcmp(s1, s2)==0)

char *get_mime_type(char* filename) {
	char *ext = get_file_extension(filename);
	if (!ext) {
		logfatal("GET Request for file (%s) has multiple '.' chars,"
		         "unable to determine MIME type!\n", filename);
		return NULL;
	}
	if (*ext == '\0' || *(ext + 1) == '\0') {
		logfatal("GET request for file (%s) has no file extension,"
		         "unable to determine MIME type!\n", filename);
		return NULL;
	}
	// extend as necessary
	if (streq(ext, ".bin")) 	return "application/octet-stream";
	if (streq(ext, ".css")) 	return "application/octet-stream";
	if (streq(ext, ".aac")) 	return "audio/aac";
	if (streq(ext, ".abw")) 	return "application/x-abiword";
	if (streq(ext, ".apng")) 	return "image/apng";
	if (streq(ext, ".arc")) 	return "application/x-freearc";
	if (streq(ext, ".avif")) 	return "image/avif";
	if (streq(ext, ".avi")) 	return "video/x-msvideo";
	if (streq(ext, ".azw")) 	return "application/vnd.amazon.ebook";
	if (streq(ext, ".bin")) 	return "application/octet-stream";
	if (streq(ext, ".bmp")) 	return "image/bmp";
	if (streq(ext, ".bz")) 		return "application/x-bzip";
	if (streq(ext, ".bz2")) 	return "application/x-bzip2";
	if (streq(ext, ".cda")) 	return "application/x-cdf";
	if (streq(ext, ".csh")) 	return "application/x-csh";
	if (streq(ext, ".css")) 	return "text/css";
	if (streq(ext, ".csv")) 	return "text/csv";
	if (streq(ext, ".doc")) 	return "application/msword";
	if (streq(ext, ".docx")) 	return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	if (streq(ext, ".eot")) 	return "application/vnd.ms-fontobject";
	if (streq(ext, ".epub")) 	return "application/epub+zip";
	if (streq(ext, ".gz")) 		return "application/gzip";
	if (streq(ext, ".gif")) 	return "image/gif";
	if (streq(ext, ".ico")) 	return "image/vnd.microsoft.icon";
	if (streq(ext, ".ics")) 	return "text/calendar";
	if (streq(ext, ".jar")) 	return "application/java-archive";
	if (streq(ext, ".js")) 		return "text/javascript";
	if (streq(ext, ".json")) 	return "application/json";
	if (streq(ext, ".jsonld")) 	return "application/ld+json";
	if (streq(ext, ".md")) 		return "text/markdown";
	if (streq(ext, ".mjs")) 	return "text/javascript";
	if (streq(ext, ".mp3")) 	return "audio/mpeg";
	if (streq(ext, ".mp4")) 	return "video/mp4";
	if (streq(ext, ".mpeg")) 	return "video/mpeg";
	if (streq(ext, ".mpkg")) 	return "application/vnd.apple.installer+xml";
	if (streq(ext, ".odp")) 	return "application/vnd.oasis.opendocument.presentation";
	if (streq(ext, ".ods")) 	return "application/vnd.oasis.opendocument.spreadsheet";
	if (streq(ext, ".odt")) 	return "application/vnd.oasis.opendocument.text";
	if (streq(ext, ".oga")) 	return "audio/ogg";
	if (streq(ext, ".ogv")) 	return "video/ogg";
	if (streq(ext, ".ogx")) 	return "application/ogg";
	if (streq(ext, ".opus")) 	return "audio/ogg";
	if (streq(ext, ".otf")) 	return "font/otf";
	if (streq(ext, ".png")) 	return "image/png";
	if (streq(ext, ".pdf")) 	return "application/pdf";
	if (streq(ext, ".php")) 	return "application/x-httpd-php";
	if (streq(ext, ".ppt")) 	return "application/vnd.ms-powerpoint";
	if (streq(ext, ".pptx")) 	return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	if (streq(ext, ".rar")) 	return "application/vnd.rar";
	if (streq(ext, ".rtf")) 	return "application/rtf";
	if (streq(ext, ".sh")) 		return "application/x-sh";
	if (streq(ext, ".svg")) 	return "image/svg+xml";
	if (streq(ext, ".tar")) 	return "application/x-tar";
	if (streq(ext, ".ts")) 		return "video/mp2t";
	if (streq(ext, ".ttf")) 	return "font/ttf";
	if (streq(ext, ".txt")) 	return "text/plain";
	if (streq(ext, ".vsd")) 	return "application/vnd.visio";
	if (streq(ext, ".wav")) 	return "audio/wav";
	if (streq(ext, ".weba")) 	return "audio/webm";
	if (streq(ext, ".webm")) 	return "video/webm";
	if (streq(ext, ".webmani")) 	return "festapplication/manifest+json";
	if (streq(ext, ".webp")) 	return "image/webp";
	if (streq(ext, ".woff")) 	return "font/woff";
	if (streq(ext, ".woff2")) 	return "font/woff2";
	if (streq(ext, ".xhtml")) 	return "application/xhtml+xml";
	if (streq(ext, ".xls")) 	return "application/vnd.ms-excel";
	if (streq(ext, ".xlsx")) 	return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	if (streq(ext, ".xml")) 	return "application/xml";
	if (streq(ext, ".xul")) 	return "application/vnd.mozilla.xul+xml";
	if (streq(ext, ".zip")) 	return "application/zip";
	if (streq(ext, ".3gp")) 	return "video/3gpp";
	if (streq(ext, ".3g2")) 	return "video/3gpp2";
	if (streq(ext, ".7z")) 		return "application/x-7z-compressed";
	if (streq(ext, ".tif")) 	return "image/tiff";
	if (streq(ext, ".tiff")) 	return "image/tiff";
	if (streq(ext, ".htm")) 	return "text/html";
	if (streq(ext, ".html")) 	return "text/html";
	if (streq(ext, ".jpeg")) 	return "image/jpeg";
	if (streq(ext, ".jpg")) 	return "image/jpeg";
	if (streq(ext, ".mid")) 	return "audio/midi";
	if (streq(ext, ".midi")) 	return "audio/midi";
	return ext;
}
