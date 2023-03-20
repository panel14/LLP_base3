#pragma once
#include <inttypes.h>

#define NODE_HEAD_SIZE sizeof(union nodeHeader)
#define NODE_SIZE sizeof(struct keyNode)

union nodeHeader {
	struct {
		uint64_t parentNode;
		uint64_t allocNode;
	};
	struct {
		uint64_t prevNode;
		uint64_t nextNode;
	};
};

struct keyNode {

	union nodeHeader header;
	uint64_t* data;
};