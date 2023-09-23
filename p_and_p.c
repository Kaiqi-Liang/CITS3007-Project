#include <assert.h>
#include <ctype.h>
#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ItemDetails ItemDetails;
typedef struct Character Character;
typedef struct ItemCarried ItemCarried;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bits");

static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls);
static int isValidCharacters(const Character *arr, size_t numEls);
static void sanitiseItemDetails(const ItemDetails *arr, size_t numEls);
static void sanitise_string(const char *buffer);
static void
sanitise_buffer(const char *buffer, size_t valid_length, size_t max_size);
static void sanitiseCharacters(const Character *arr, size_t numEls);
static int loadCharacter(Character *character, int fd);
static int saveCharacter(Character character, int fd);

int saveItemDetails(const ItemDetails *arr, size_t numEls, int fd) {
	ssize_t res;
	if ((res = write(fd, &numEls, sizeof(numEls))) == -1 ||
	    (size_t)res != sizeof(numEls))
	{
		return EXIT_FAILURE;
	}
	if (isValidItemDetailsAll(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	sanitiseItemDetails(arr, numEls);
	const size_t size = sizeof(ItemDetails) * numEls;
	if ((res = write(fd, arr, size)) == -1 || (size_t)res != size) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidItemDetails(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static void sanitiseItemDetails(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitise_string(arr[i].name);
		sanitise_string(arr[i].desc);
	}
}

static void sanitise_string(const char *buffer) {
	size_t length = strlen(buffer);
	sanitise_buffer(buffer, length, DEFAULT_BUFFER_SIZE);
}

static void
sanitise_buffer(const char *buffer, size_t valid_length, size_t max_size) {
	memset(buffer + valid_length, '\0', max_size - valid_length);
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
	if ((res = read(fd, numEls, sizeof(numEls))) == -1 ||
	    (size_t)res != sizeof(numEls))
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
		free(*ptr);
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

int isValidCharacter(const Character *c) {
	if (isValidName(c->profession) == EXIT_SUCCESS &&
	    isValidMultiword(c->name) == EXIT_SUCCESS &&
	    c->inventorySize <= MAX_ITEMS)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int saveCharacters(Character *arr, size_t numEls, int fd) {
	ssize_t res;
	if ((res = write(fd, &numEls, sizeof(numEls))) == -1 ||
	    (size_t)res != sizeof(numEls))
	{
		return EXIT_FAILURE;
	}
	if (isValidCharacters(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	sanitiseCharacters(arr, numEls);
	for (size_t i = 0; i < numEls; ++i) {
		saveCharacter(arr[i], fd);
	}
	return EXIT_SUCCESS;
}

static int isValidCharacters(const Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidCharacter(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static void sanitiseCharacters(const Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitise_string(arr[i].name);
		sanitise_string(arr[i].profession);
		sanitise_buffer(
		    arr[i].profession,
		    sizeof(ItemCarried) * arr[i].inventorySize,
		    sizeof(ItemCarried) * MAX_ITEMS
		);
	}
}

static int saveCharacter(Character character, int fd) {
	ssize_t res;
	if ((res = write(fd, &character.characterID, sizeof(character.characterID))
	    ) == -1 ||
	    (size_t)res != sizeof(character.characterID))
	{
		return EXIT_FAILURE;
	}
	if ((res = write(fd, &character.socialClass, sizeof(character.socialClass))
	    ) == -1 ||
	    (size_t)res != sizeof(character.socialClass))
	{
		return EXIT_FAILURE;
	}
	if ((res = write(fd, character.profession, sizeof(character.profession))) ==
	        -1 ||
	    (size_t)res != sizeof(character.profession))
	{
		return EXIT_FAILURE;
	}
	if ((res = write(fd, character.name, sizeof(character.name))) == -1 ||
	    (size_t)res != sizeof(character.name))
	{
		return EXIT_FAILURE;
	}
	if ((res = write(
	         fd,
	         &character.inventorySize,
	         sizeof(character.inventorySize)
	     )) == -1 ||
	    (size_t)res != sizeof(character.inventorySize))
	{
		return EXIT_FAILURE;
	}
	const size_t size = sizeof(ItemCarried) * character.inventorySize;
	if ((res = write(fd, character.inventory, size)) == -1 ||
	    (size_t)res != size)
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int loadCharacters(Character **ptr, size_t *numEls, int fd) {
	ssize_t res;
	if ((res = read(fd, numEls, sizeof(numEls))) == -1 ||
	    (size_t)res != sizeof(numEls))
	{
		return EXIT_FAILURE;
	}
	const size_t size = sizeof(Character) * *numEls;
	*ptr = malloc(size);
	for (size_t i = 0; i < *numEls; ++i) {
		if (loadCharacter(&(*ptr)[i], fd) == EXIT_FAILURE) {
			free(*ptr);
			return EXIT_FAILURE;
		}
	}
	if (isValidCharacters(*ptr, *numEls) == EXIT_FAILURE) {
		free(*ptr);
		return EXIT_FAILURE;
	}
	sanitiseCharacters(*ptr, *numEls);
	return EXIT_SUCCESS;
}

static int loadCharacter(Character *character, int fd) {
	ssize_t res;
	if ((res = read(fd, &character->characterID, sizeof(character->characterID))
	    ) == -1 ||
	    (size_t)res != sizeof(character->characterID))
	{
		return EXIT_FAILURE;
	}
	if ((res = read(fd, &character->socialClass, sizeof(character->socialClass))
	    ) == -1 ||
	    (size_t)res != sizeof(character->socialClass))
	{
		return EXIT_FAILURE;
	}
	if ((res = read(fd, character->profession, sizeof(character->profession))
	    ) == -1 ||
	    (size_t)res != sizeof(character->profession))
	{
		return EXIT_FAILURE;
	}
	if ((res = read(fd, character->name, sizeof(character->name))) == -1 ||
	    (size_t)res != sizeof(character->name))
	{
		return EXIT_FAILURE;
	}
	if ((res = read(
	         fd,
	         &character->inventorySize,
	         sizeof(character->inventorySize)
	     )) == -1 ||
	    (size_t)res != sizeof(character->inventorySize))
	{
		return EXIT_FAILURE;
	}
	const size_t size = sizeof(ItemCarried) * character->inventorySize;
	if ((res = read(fd, character->inventory, size)) == -1 ||
	    (size_t)res != size)
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int secureLoad(const char *filepath) {
	return 0;
}

void playGame(ItemDetails *ptr, size_t numEls);
