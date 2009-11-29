/* Yash: yet another shell */
/* trie.c: trie library for lineedit */
/* (C) 2007-2009 magicant */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include "../common.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "../strbuf.h"
#include "../util.h"
#include "trie.h"


typedef union triekey_T {
    char as_char;
    wchar_t as_wchar;
} triekey_T;
typedef struct trieentry_T {
    triekey_T key;
    struct trienode_T *child;
} trieentry_T;
typedef struct trienode_T {
    bool valuevalid;
    trievalue_T value;
    size_t count;
    struct trieentry_T entries[];
} trienode_T;


#define RAISE_COUNT(count) ((count) | 3)
#define NODE_SIZE(count)   (sizeof(trienode_T) + (count) * sizeof(trieentry_T))

static inline bool isempty(const trienode_T *node)
    __attribute__((nonnull,pure));
static trienode_T *ensure_size(trienode_T *node, size_t count)
    __attribute__((nonnull,malloc,warn_unused_result));
static inline ssize_t search(const trienode_T *node, char c)
    __attribute__((nonnull,pure));
static ssize_t binarysearch(const trieentry_T *e, size_t count, char key)
    __attribute__((nonnull,pure));
static inline ssize_t searchw(const trienode_T *node, wchar_t c)
    __attribute__((nonnull,pure));
static ssize_t binarysearchw(const trieentry_T *e, size_t count, wchar_t key)
    __attribute__((nonnull,pure));
static trienode_T *insert_entry(trienode_T *node, size_t index, triekey_T key)
    __attribute__((nonnull,malloc,warn_unused_result));
static trienode_T *shrink(trienode_T *node)
    __attribute__((nonnull,malloc,warn_unused_result));
static int foreachw(const trienode_T *t,
	int (*func)(void *v, const wchar_t *key, le_command_func_T *cmd),
	void *v,
	xwcsbuf_T *buf)
    __attribute__((nonnull(1,2,4)));


/* Creates a new empty trie. */
trienode_T *trie_create(void)
{
    trienode_T *result = xmalloc(NODE_SIZE(RAISE_COUNT(0)));
    result->valuevalid = false;
    result->count = 0;
    return result;
}

/* Checks if the node is empty. */
/* An empty node can be freed. */
bool isempty(const trienode_T *node)
{
    return node->count == 0 && !node->valuevalid;
}

/* Reallocates the node to ensure the enough node size for the specified entry
 * count. */
trienode_T *ensure_size(trienode_T *node, size_t count)
{
    count = RAISE_COUNT(count);
    if (RAISE_COUNT(node->count) >= count)
	return node;
    else
	return xrealloc(node, NODE_SIZE(count));
}

/* Search the node for the entry of the specified byte character key.
 * If the entry is found, the index to it is returned.
 * If not found, the negation of the index that the not-found entry would have
 * minus one is returned. */
ssize_t search(const trienode_T *node, char key)
{
    return binarysearch(node->entries, node->count, key);
}

ssize_t binarysearch(const trieentry_T *e, size_t count, char key)
{
    ssize_t offset = 0;

    while (count > 0) {
	size_t i = count / 2;
	char ekey = e[offset + i].key.as_char;

	if (key == ekey)
	    return offset + i;
	else if (key < ekey)
	    count = i;
	else
	    offset += i + 1, count -= i + 1;
    }
    return -offset - 1;
}

/* Search the node for the entry of the specified wide character key.
 * If the entry is found, the index to it is returned.
 * If not found, the negation of the index that the not-found entry would have
 * minus one is returned. */
ssize_t searchw(const trienode_T *node, wchar_t key)
{
    return binarysearchw(node->entries, node->count, key);
}

ssize_t binarysearchw(const trieentry_T *e, size_t count, wchar_t key)
{
    ssize_t offset = 0;

    while (count > 0) {
	size_t i = count / 2;
	wchar_t ekey = e[offset + i].key.as_wchar;

	if (key == ekey)
	    return offset + i;
	else if (key < ekey)
	    count = i;
	else
	    offset += i + 1, count -= i + 1;
    }
    return -offset - 1;
}

/* Creates a new child at `index'. */
trienode_T *insert_entry(trienode_T *node, size_t index, triekey_T key)
{
    assert(index <= node->count);
    node = ensure_size(node, node->count + 1);
    if (index < node->count)
	memmove(node->entries + index + 1, node->entries + index,
		sizeof(trieentry_T) * (node->count - index));
    node->entries[index].key = key;
    node->entries[index].child = trie_create();
    node->count++;
    return node;
}

/* Adds a mapping from `keystr' to `v'.
 * The existing mapping for `keystr' is lost if any. */
trienode_T *trie_set(trienode_T *node, const char *keystr, trievalue_T v)
{
    if (keystr[0] == '\0') {
	node->valuevalid = true;
	node->value = v;
    } else {
	ssize_t index = search(node, keystr[0]);

	if (index < 0) {
	    index = -(index + 1);
	    node = insert_entry(node, (size_t) index,
		    (triekey_T) { .as_char = keystr[0] });
	}
	node->entries[index].child =
	    trie_set(node->entries[index].child, keystr + 1, v);
    }
    return node;
}

/* Adds a mapping from "\0" to `v'.
 * The existing mapping for `keystr' is lost if any. */
trienode_T *trie_set_null(trienode_T *node, trievalue_T v)
{
    ssize_t index = search(node, '\0');

    if (index < 0) {
	index = -(index + 1);
	node = insert_entry(node, (size_t) index,
		(triekey_T) { .as_char = '\0' });
    }
    node->entries[index].child =
	trie_set(node->entries[index].child, "", v);
    return node;
}

/* Adds a mapping from `keywcs' to `v'.
 * The existing mapping for `keywcs' is lost if any. */
trienode_T *trie_setw(trienode_T *node, const wchar_t *keywcs, trievalue_T v)
{
    if (keywcs[0] == L'\0') {
	node->valuevalid = true;
	node->value = v;
    } else {
	ssize_t index = searchw(node, keywcs[0]);

	if (index < 0) {
	    index = -(index + 1);
	    node = insert_entry(node, (size_t) index,
		    (triekey_T) { .as_char = keywcs[0] });
	}
	node->entries[index].child =
	    trie_setw(node->entries[index].child, keywcs + 1, v);
    }
    return node;
}

/* Decreases the child count. The node may be reallocated in this function. */
trienode_T *shrink(trienode_T *node)
{
    size_t oldcount, newcount;

    oldcount = RAISE_COUNT(node->count);
    node->count--;
    newcount = RAISE_COUNT(node->count);
    if (oldcount != newcount)
	node = xrealloc(node, NODE_SIZE(newcount));
    return node;
}

/* Removes the mapping for the specified byte string key. */
trienode_T *trie_remove(trienode_T *node, const char *keystr)
{
    if (keystr[0] == '\0') {
	node->valuevalid = false;
    } else {
	ssize_t index = search(node, keystr[0]);

	if (index >= 0) {
	    node->entries[index].child =
		trie_remove(node->entries[index].child, keystr + 1);
	    if (isempty(node->entries[index].child)) {
		free(node->entries[index].child);
		memmove(node->entries + index, node->entries + index + 1,
			sizeof(trieentry_T) * (node->count - index - 1));
		node = shrink(node);
	    }
	}
    }
    return node;
}

/* Removes the mapping for the specified wide string key. */
trienode_T *trie_removew(trienode_T *node, const wchar_t *keywcs)
{
    if (keywcs[0] == L'\0') {
	node->valuevalid = false;
    } else {
	ssize_t index = searchw(node, keywcs[0]);

	if (index >= 0) {
	    node->entries[index].child =
		trie_removew(node->entries[index].child, keywcs + 1);
	    if (isempty(node->entries[index].child)) {
		free(node->entries[index].child);
		memmove(node->entries + index, node->entries + index + 1,
			sizeof(trieentry_T) * (node->count - index - 1));
		node = shrink(node);
	    }
	}
    }
    return node;
}

/* Matches `keystr' with the entries of the trie.
 * This function does not treat '\0' as the end of a string. The length of
 * `keystr' is given by `keylen'.
 * `value' of the return value is valid only if the TG_EXACTMATCH flag is in
 * `type'. */
trieget_T trie_get(const trienode_T *t, const char *keystr, size_t keylen)
{
    trieget_T result;

    if (keylen == 0) {
	result.type = TG_NOMATCH, result.matchlength = 0;
	if (t->valuevalid)
	    result.type |= TG_EXACTMATCH, result.value = t->value;
	if (t->count > 0)
	    result.type |= TG_PREFIXMATCH;
	return result;
    }

    ssize_t index = search(t, keystr[0]);
    if (index < 0) {
	result.type = TG_NOMATCH, result.matchlength = 0;
	if (t->valuevalid)
	    result.type |= TG_EXACTMATCH, result.value = t->value;
	return result;
    }

    result = trie_get(t->entries[index].child, keystr + 1, keylen - 1);
    if (result.type & TG_EXACTMATCH) {
	result.matchlength++;
    } else if (t->valuevalid) {
	result.type |= TG_EXACTMATCH;
	result.matchlength = 0;
	result.value = t->value;
    }
    return result;
}

/* Matches `keywcs' with the entries of the trie.
 * `value' of the return value is valid only if the TG_EXACTMATCH flag is in
 * `type'. */
trieget_T trie_getw(const trienode_T *t, const wchar_t *keywcs)
{
    trieget_T result;

    if (keywcs[0] == L'\0') {
	result.type = TG_NOMATCH, result.matchlength = 0;
	if (t->valuevalid)
	    result.type |= TG_EXACTMATCH, result.value = t->value;
	if (t->count > 0)
	    result.type |= TG_PREFIXMATCH;
	return result;
    }

    ssize_t index = searchw(t, keywcs[0]);
    if (index < 0) {
	result.type = TG_NOMATCH, result.matchlength = 0;
	if (t->valuevalid)
	    result.type |= TG_EXACTMATCH, result.value = t->value;
	return result;
    }

    result = trie_getw(t->entries[index].child, keywcs + 1);
    if (result.type & TG_EXACTMATCH) {
	result.matchlength++;
    } else if (t->valuevalid) {
	result.type |= TG_EXACTMATCH;
	result.matchlength = 0;
	result.value = t->value;
    }
    return result;
}

/* Calls function `func' for each entry in trie `t'.
 * Pointer `v' is passed to the function as the first argument.
 * Returns zero if `func' was called for all the entries.
 * When `func' returns non-zero, the iteration is aborted and `trie_foreachw'
 * returns the value returned by `func'. */
int trie_foreachw(const trienode_T *t,
	int (*func)(void *v, const wchar_t *key, le_command_func_T *cmd),
	void *v)
{
    xwcsbuf_T buf;
    int result;

    wb_init(&buf);
    result = foreachw(t, func, v, &buf);
    wb_destroy(&buf);
    return result;
}

int foreachw(const trienode_T *t,
	int func(void *v, const wchar_t *key, le_command_func_T cmd),
	void *v,
	xwcsbuf_T *buf)
{
    int result;

    if (t->valuevalid) {
	result = func(v, buf->contents, t->value.cmdfunc);
	if (result != 0)
	    return result;
    }

    for (size_t i = 0; i < t->count; i++) {
	wb_wccat(buf, t->entries[i].key.as_wchar);

	result = foreachw(t->entries[i].child, func, v, buf);
	if (result != 0)
	    return result;

	assert(buf->length > 0);
	wb_truncate(buf, buf->length - 1);
    }

    return 0;
}

/* Destroys the whole tree.
 * All the stored data are lost. */
void trie_destroy(trienode_T *node)
{
    if (node) {
	for (size_t i = 0; i < node->count; i++)
	    trie_destroy(node->entries[i].child);
	free(node);
    }
}


/* vim: set ts=8 sts=4 sw=4 noet tw=80: */
