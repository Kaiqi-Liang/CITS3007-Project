#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef ssize_t (*ioFuncPtr)(int, void *, size_t); // can only be read(2) or write(2)
typedef struct ItemDetails ItemDetails;
typedef struct Character Character;
typedef struct ItemCarried ItemCarried;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bit unsigned int");

static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc);
static int processCharacter(Character *character, int fd, ioFuncPtr ioFunc);
static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls);
static int isValidCharacters(const Character *arr, size_t numEls);
static void sanitiseItemDetails(ItemDetails *arr, size_t numEls);
static void sanitiseCharacters(Character *arr, size_t numEls);
static void sanitiseString(char *buffer);
static void sanitiseBuffer(void *buffer, size_t valid_length, size_t max_size);

/**
 * @brief Serialise an array of `ItemDetails` structs and store the array using
 * the `ItemDetails` file format
 */
int saveItemDetails(ItemDetails *arr, size_t numEls, int fd) {
	if (processField(fd, &numEls, sizeof(numEls), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidItemDetailsAll(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	sanitiseItemDetails(arr, numEls);

	if (processField(fd, arr, sizeof(*arr) * numEls, (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * @brief Deseriaslise an array of `ItemDetails` structs from a file descriptor
 */
int loadItemDetails(ItemDetails **ptr, size_t *numEls, int fd) {
	if (processField(fd, numEls, sizeof(*numEls), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(ItemDetails) * *numEls;
	*ptr = malloc(size);
	if (ptr == NULL) {
		return EXIT_FAILURE;
	}

	if (processField(fd, *ptr, size, read) == EXIT_FAILURE) {
		goto err;
	}

	if (isValidItemDetailsAll(*ptr, *numEls) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseItemDetails(*ptr, *numEls);
	return EXIT_SUCCESS;

err: // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

/**
 * @brief Check whether a string constitutes a valid `name` field
 */
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

/**
 * @brief Check whether a string constitutes a valid `multi-word` field
 */
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

/**
 * @brief Check whether an `ItemDetails` struct is valid
 */
int isValidItemDetails(const ItemDetails *id) {
	if (isValidName(id->name) == EXIT_SUCCESS && isValidMultiword(id->desc) == EXIT_SUCCESS) {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/**
 * @brief Check whether a `Character` struct is valid
 */
int isValidCharacter(const Character *c) {
	if (isValidName(c->profession) == EXIT_SUCCESS && isValidMultiword(c->name) == EXIT_SUCCESS &&
	    c->inventorySize <= MAX_ITEMS)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/**
 * @brief Serialise an array of `Character` structs and store the array using
 * the `Character` file format
 */
int saveCharacters(Character *arr, size_t numEls, int fd) {
	if (processField(fd, &numEls, sizeof(numEls), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidCharacters(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	sanitiseCharacters(arr, numEls);

	for (size_t i = 0; i < numEls; ++i) {
		processCharacter(&arr[i], fd, (ioFuncPtr)write);
	}

	return EXIT_SUCCESS;
}

/**
 * @brief Deseriaslise an array of `Character` structs from a file descriptor
 */
int loadCharacters(Character **ptr, size_t *numEls, int fd) {
	if (processField(fd, numEls, sizeof(*numEls), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(Character) * *numEls;
	*ptr = malloc(size);
	if (ptr == NULL) {
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < *numEls; ++i) {
		if (processCharacter(&(*ptr)[i], fd, read) == EXIT_FAILURE) {
			goto err;
		}
	}

	if (isValidCharacters(*ptr, *numEls) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseCharacters(*ptr, *numEls);
	return EXIT_SUCCESS;

err: // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

/**
 * @brief Acquire appropriate permissions, load the `ItemDetails` database, and
 * then permanently drop permissions
 */
int secureLoad(const char *filepath) {
	const int fd = open(filepath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	size_t numEls;
	ItemDetails *item_details;
	if (loadItemDetails(&item_details, &numEls, fd) == EXIT_FAILURE) {
		return 1;
	}
	if (setuid(getuid()) == -1) {
		return 2;
	}
	playGame(item_details, numEls);
	return 0;
}

/**
 * @brief Start playing the game
 */
void playGame(struct ItemDetails *ptr, size_t numEls) {}

/**
 * @brief Read/write from/to a file to/from a buffer of a given size
 */
static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc) {
	const ssize_t res = ioFunc(fd, buf, size);
	if (res == -1 || (size_t)res != size) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Read/write each of the field in a `Character` struct from/to a file
 */
static int processCharacter(Character *character, int fd, ioFuncPtr ioFunc) {
	if (processField(fd, &character->characterID, sizeof(character->characterID), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, &character->socialClass, sizeof(character->socialClass), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, character->profession, sizeof(character->profession), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, character->name, sizeof(character->name), ioFunc) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (processField(fd, &character->inventorySize, sizeof(character->inventorySize), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(
	        fd,
	        character->inventory,
	        sizeof(ItemCarried) * character->inventorySize,
	        ioFunc
	    ) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/**
 * @brief Check whether an array of `ItemDetails` structs are valid
 */
static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidItemDetails(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Check whether an array of `Character` structs are valid
 */
static int isValidCharacters(const Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidCharacter(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Sanitise all the string fields in a `ItemDetails` struct
 */
static void sanitiseItemDetails(ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitiseString(arr[i].name);
		sanitiseString(arr[i].desc);
	}
}

/**
 * @brief Sanitise all the string fields in a `ItemDetails` struct and the
 * `inventory` field
 */
static void sanitiseCharacters(Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitiseString(arr[i].name);
		sanitiseString(arr[i].profession);
		sanitiseBuffer(
		    arr[i].inventory,
		    sizeof(ItemCarried) * arr[i].inventorySize,
		    sizeof(ItemCarried) * MAX_ITEMS
		);
	}
}

/**
 * @brief Sanitise everything after the nul terminator
 */
static void sanitiseString(char *buffer) {
	size_t length = strlen(buffer);
	sanitiseBuffer(buffer, length, DEFAULT_BUFFER_SIZE);
}

/**
 * @brief Fill the buffer from after the valid point to the end with `0`s
 */
static void sanitiseBuffer(void *buffer, size_t valid_length, size_t max_size) {
	memset(buffer + valid_length, 0, max_size - valid_length);
}
