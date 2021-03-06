
#include "PancakeHTTPDeflate.h"
#include "../PancakeConfiguration.h"
#include "../PancakeLogger.h"

STATIC UByte PancakeHTTPDeflateChunk(PancakeSocket *sock, String *chunk);
STATIC UByte PancakeHTTPDeflateInitialize();
STATIC void PancakeHTTPDeflateOnOutputEnd(PancakeHTTPRequest *request);

PancakeModule PancakeHTTPDeflate = {
		"HTTPDeflate",

		PancakeHTTPDeflateInitialize,
		NULL,
		NULL,

		0
};

PancakeHTTPOutputFilter PancakeHTTPDeflateFilter = {
		"Deflate",
		PancakeHTTPDeflateChunk,

		NULL
};

PancakeHTTPDeflateConfigurationStructure PancakeHTTPDeflateConfiguration;

static String PancakeHTTPDeflateContentEncoding = {
		"deflate",
		sizeof("deflate") - 1
};

STATIC UByte PancakeHTTPDeflateInitialize() {
	PancakeConfigurationGroup *HTTP, *vHostGroup, *group;
	PancakeConfigurationSetting *vHost;

	// Defer if HTTP module is not yet initialized
	if(!PancakeHTTP.initialized) {
		return 2;
	}

	// Register deflate filter
	PancakeHTTPRegisterOutputFilter(&PancakeHTTPDeflateFilter);

	HTTP = PancakeConfigurationLookupGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1});
	vHost = PancakeConfigurationLookupSetting(HTTP, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1});
	vHostGroup = vHost->listGroup;
	group = PancakeConfigurationAddGroup(HTTP, (String) {"Deflate", sizeof("Deflate") - 1}, NULL);
	PancakeConfigurationAddSetting(group, (String) {"Level", sizeof("Level") - 1}, CONFIG_TYPE_INT, &PancakeHTTPDeflateConfiguration.level, sizeof(Int32), (config_value_t) 0, NULL);
	PancakeConfigurationAddSetting(group, (String) {"WindowBits", sizeof("WindowBits") - 1}, CONFIG_TYPE_INT, &PancakeHTTPDeflateConfiguration.windowBits, sizeof(Int32), (config_value_t) -15, NULL);
	PancakeConfigurationAddSetting(group, (String) {"MemoryLevel", sizeof("MemoryLevel") - 1}, CONFIG_TYPE_INT, &PancakeHTTPDeflateConfiguration.memoryLevel, sizeof(Int32), (config_value_t) 9, NULL);

	// Deflate -> vHost configuration
	PancakeConfigurationAddGroupToGroup(vHostGroup, group);

	return 1;
}

STATIC void PancakeHTTPDeflateOnOutputEnd(PancakeHTTPRequest *request) {
	deflateEnd(request->outputFilterData);
	PancakeFree(request->outputFilterData);
}

STATIC UByte PancakeHTTPDeflateChunk(PancakeSocket *sock, String *chunk) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	z_streamp stream;
	String output;
	UByte out[chunk->length + 16];

	// Is there already an existing deflate stream for this request?
	if(!request->outputFilterData) {
		UByte *d;

		// Check whether client accepts deflate encoding
		if(request->HTTPVersion == PANCAKE_HTTP_11 // No support for deflate in HTTP 1.0 and doesn't make sense anyway (no chunked TE)
		&& PancakeHTTPDeflateConfiguration.level // Is deflate enabled?
		&& !request->contentEncoding // Is the content already encoded?
		&& request->acceptEncoding.length
		&& (d = memchr(sock->readBuffer.value + request->acceptEncoding.offset, 'd', request->acceptEncoding.length))
		&& sock->readBuffer.value + request->acceptEncoding.offset + request->acceptEncoding.length - d >= sizeof("eflate") - 1
		&& !memcmp(d + 1, "eflate", sizeof("eflate") - 1)) {
			request->outputFilterData = PancakeAllocate(sizeof(z_stream));
			stream = (z_streamp) request->outputFilterData;

			// Initialize deflate stream
			stream->zalloc = NULL;
			stream->zfree = NULL;
			stream->opaque = NULL;

			if(deflateInit2(stream, PancakeHTTPDeflateConfiguration.level, Z_DEFLATED, PancakeHTTPDeflateConfiguration.windowBits, PancakeHTTPDeflateConfiguration.memoryLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
				PancakeFree(stream);
				return 0;
			}

			// Set Content-Encoding and chunked Transfer-Encoding
			request->onOutputEnd = PancakeHTTPDeflateOnOutputEnd;
			request->chunkedTransfer = 1;
			request->contentEncoding = &PancakeHTTPDeflateContentEncoding;
		} else {
			// Client does not accept deflate coding
			return 0;
		}
	} else {
		// Use existing stream
		stream = (z_streamp) request->outputFilterData;
	}

	// Fill stream
	stream->avail_out = chunk->length + 16;
	stream->avail_in = chunk->length;
	stream->next_in = chunk->value;
	stream->next_out = out;

	// Compress data
	deflate(stream, Z_SYNC_FLUSH);

	output.value = out;
	output.length = chunk->length - stream->avail_out + 16;

	// Show debug information
	PancakeDebug {
		PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "%lu bytes deflated to %lu bytes (%.2f%%)", chunk->length, output.length, (double) output.length / chunk->length * 100);
	}

	// Make chunk out of compressed data
	PancakeHTTPSendChunk(sock, &output);

	return 1;
}
