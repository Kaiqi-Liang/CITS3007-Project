#include <assert.h>
#include <ctype.h>
#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ItemDetails ItemDetails;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bits");

static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidItemDetails(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static void sanitise_buffer(const char* buffer) {
	size_t length = strlen(buffer);
	memset(buffer + length, '\0', DEFAULT_BUFFER_SIZE - length);
}

static void sanitiseItemDetails(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitise_buffer(arr[i].name);
		sanitise_buffer(arr[i].desc);
	}
}

int saveItemDetails(const ItemDetails *arr, size_t numEls, int fd) {
	ssize_t res;
	if ((res = write(fd, &numEls, sizeof(size_t))) == -1 ||
	    (size_t)res != sizeof(size_t))
	{
		return EXIT_FAILURE;
	}
	if (isValidItemDetailsAll(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	sanitiseItemDetails(arr, numEls);
	if ((res = write(fd, arr, sizeof(ItemDetails) * numEls)) == -1 ||
	    (size_t)res != sizeof(ItemDetails) * numEls)
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int saveItemDetailsToPath(
    const ItemDetails *arr,
    size_t numEls,
    const char *filename
) {
	return 0;
}

int loadItemDetails(ItemDetails **ptr, size_t *numEls, int fd) {
	ssize_t res;
	if ((res = read(fd, numEls, sizeof(size_t))) == -1 ||
	    (size_t)res != sizeof(size_t))
	{
		return EXIT_FAILURE;
	}
	const size_t size = sizeof(ItemDetails) * *numEls;
	*ptr = malloc(size);
	if ((res = read(fd, *ptr, size)) == -1 || (size_t)res != size) {
		free(ptr);
		return EXIT_FAILURE;
	}
	if (isValidItemDetailsAll(*ptr, *numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	sanitiseItemDetails(*ptr, *numEls);
	return EXIT_SUCCESS;
}

int isValidName(const char *str) {
	for (size_t i = 0; i < DEFAULT_BUFFER_SIZE; i++) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				return EXIT_SUCCESS;
			} else if (!isgraph(str[i])) {
				return EXIT_FAILURE;
			}
		} else if (str[i] != '\0') {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int isValidMultiword(const char *str) {
	size_t i = 0;
	for (; i < DEFAULT_BUFFER_SIZE; i++) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				break;
			} else if (str[i] == ' ') {
				if (i == 0) {
					return EXIT_FAILURE;
				}
			} else if (!isgraph(str[i])) {
				return EXIT_FAILURE;
			}
		} else if (str[i] != '\0') {
			return EXIT_FAILURE;
		} else {
			break;
		}
	}
	if (i > 0 && str[i - 1] == ' ') {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int isValidItemDetails(const ItemDetails *id) {
	if (isValidName(id->name) == EXIT_SUCCESS &&
	    isValidMultiword(id->desc) == EXIT_SUCCESS)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int isValidCharacter(const struct Character *c) {
	if (isValidName(c->profession) == EXIT_SUCCESS &&
	    isValidMultiword(c->name) == EXIT_SUCCESS &&
		c->inventorySize <= MAX_ITEMS)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int saveCharacters(struct Character *arr, size_t numEls, int fd) {
	return 0;
}

int loadCharacters(struct Character **ptr, size_t *numEls, int fd) {
	return 0;
}

int secureLoad(const char *filepath) {
	return 0;
}

void playGame(ItemDetails *ptr, size_t numEls);
