#ifndef RETROS_BROWSER_HTML_HPP
#define RETROS_BROWSER_HTML_HPP

#include <lib/printf.h>
#include <libc.h>
#include <lib/syscall.h>

static const size_t MAX_TAG_NAME_LENGTH = 10;
static const size_t MAX_ATTR_NAME_LENGTH = 15;
static const size_t MAX_ATTR_VALUE_LENGTH = 256; /* hrefs can be long, keep generous to avoid parse errors. */
static const size_t MAX_ATTRIBUTES = 5;
static const size_t MAX_NODE_DATA_LENGTH = 100;

enum HTMLTag {
    Unknown,
    Html,
    Body,
    P,
    Input,
    Checkbox,
    Button,
    Label,
    Layout,
    Spacing,
    A,
    Div,
    H1,
    H2,
    H3,
    H4,
    H5,
    H6,
    Head,
    Header,
    Title,
    Ul,
    Li
};

struct TagMapping {
    HTMLTag tag;
    const char* name;
};

constexpr TagMapping kTagMappings[] = {
    {Html, "html"}, {Body, "body"}, {P, "p"},         {Input, "input"}, {Checkbox, "checkbox"},
    {Button, "button"}, {Label, "label"}, {Layout, "layout"}, {Spacing, "spacing"}, {A, "a"},
    {Div, "div"},  {H1, "h1"}, {H2, "h2"}, {H3, "h3"}, {H4, "h4"}, {H5, "h5"}, {H6, "h6"},
    {Head, "head"}, {Header, "header"}, {Title, "title"}, {Ul, "ul"}, {Li, "li"},
};

inline bool stringsEqual(const char* a, const char* b) {
    if (a == NULL || b == NULL) {
        return false;
    }
    const size_t lenA = (size_t)strlen(a);
    const size_t lenB = (size_t)strlen(b);
    if (lenA != lenB) {
        return false;
    }
    return memcmp(a, b, (uint32_t)lenA) == 0;
}

inline bool copyToBuffer(const char* source, size_t length, char* destination, size_t destinationSize) {
    if (destinationSize == 0 || destination == nullptr || source == nullptr) {
        return false;
    }

    const int copyLength = (int)(length < destinationSize - 1 ? length : destinationSize - 1);
    memcpy(destination, source, copyLength);
    destination[copyLength] = '\0';
    return (size_t)copyLength == length;
}

inline HTMLTag stringToHTMLTag(const char* str) {
    for (const auto& mapping : kTagMappings) {
        if (stringsEqual(str, mapping.name)) {
            return mapping.tag;
        }
    }
    return Unknown;
}

inline const char* htmlTagToString(HTMLTag tag) {
    for (const auto& mapping : kTagMappings) {
        if (mapping.tag == tag) {
            return mapping.name;
        }
    }
    return "unknown";
}

struct HTMLAttribute {
    char name[MAX_ATTR_NAME_LENGTH];
    char value[MAX_ATTR_VALUE_LENGTH];
};

struct Node {
    HTMLTag tag;

    HTMLAttribute attributes[MAX_ATTRIBUTES];
    int attr_count;

    char data[MAX_NODE_DATA_LENGTH];

    bool isTag;

    Node* parent;
    Node* firstChild;
    Node* nextSibling;

    Node(HTMLTag tag, const char* data, bool isTagNode, Node* parentNode)
        : tag(tag), attr_count(0), isTag(isTagNode), parent(parentNode), firstChild(nullptr), nextSibling(nullptr) {
        copyToBuffer(data, (size_t)strlen(data), this->data, MAX_NODE_DATA_LENGTH);
    }

    /* Children are freed iteratively by HTMLParser to avoid deep recursion on large documents. */
    ~Node() = default;

    bool addAttribute(const char* name, const char* value) {
        if (attr_count >= static_cast<int>(MAX_ATTRIBUTES)) {
            printf("Too many attributes on node\n");
            return false;
        }

        if (!copyToBuffer(name, (size_t)strlen(name), attributes[attr_count].name, MAX_ATTR_NAME_LENGTH)) {
            printf("Attribute name too long\n");
            return false;
        }

        if (!copyToBuffer(value, (size_t)strlen(value), attributes[attr_count].value, MAX_ATTR_VALUE_LENGTH)) {
            printf("Attribute value too long\n");
            return false;
        }

        attr_count++;
        return true;
    }
};

class HTMLParser {
public:
    explicit HTMLParser(const char* htmlContent)
        : html(htmlContent ? htmlContent : ""), root(new Node(Unknown, "root", true, nullptr)), current(root) {}

    ~HTMLParser() { freeTree(root); }

    int parse() {
        const char* pos = html;
        while (*pos != '\0') {
            pos = skipWhitespace(pos);
            if (*pos == '<') {
                if (*(pos + 1) != '/') {
                    const char* tagStart = pos + 1;
                    pos = findTagEnd(pos);
                    if (*pos == '\0') {
                        printf("Malformed HTML: Tag not closed\n");
                        return -1;
                    }

                    char tagStr[MAX_TAG_NAME_LENGTH] = {0};
                    bool tagNameValid = true;
                    const char* tagEnd = extractTagName(tagStart, tagStr, tagNameValid);
                    if (!tagNameValid) {
                        return -1;
                    }
                    HTMLTag tag = stringToHTMLTag(tagStr);

                    Node* child = new Node(tag, "", true, current);
                    if (parseAttributes(tagEnd, pos, child) != 0) {
                        delete child;
                        return -2;
                    }

                    addChild(child);
                    current = child;
                } else {
                    pos = findTagEnd(pos);
                    if (*pos == '\0') return -1;
                    current = current->parent ? current->parent : current;
                }
            } else {
                const char* contentStart = pos;
                while (*pos != '<' && *pos != '\0') {
                    pos++;
                }

                if (pos > contentStart) {
                    char content[MAX_NODE_DATA_LENGTH] = {0};
                    copyToBuffer(contentStart, (size_t)(pos - contentStart), content, MAX_NODE_DATA_LENGTH);
                    Node* child = new Node(Unknown, content, false, current);
                    addChild(child);
                }
            }
        }
        return 0;
    }

    void printTree() const { printTreeIterative(); }

    static const char* getHtmlError(int code) {
        switch (code) {
        case -1:
            return "Malformed HTML";
        case -2:
            return "Error parsing attributes";
        default:
            return "Unknown error";
        }
    }

private:
    const char* html;
    Node* root;
    Node* current;

    bool isspace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    const char* findTagEnd(const char* start) const {
        while (*start != '>' && *start != '\0') {
            start++;
        }
        return *start == '>' ? start + 1 : start;
    }

    const char* skipWhitespace(const char* start) const {
        while (*start != '\0' && isspace(*start)) {
            start++;
        }
        return start;
    }

    void addChild(Node* child) {
        if (!current->firstChild) {
            current->firstChild = child;
        } else {
            Node* temp = current->firstChild;
            while (temp->nextSibling) {
                temp = temp->nextSibling;
            }
            temp->nextSibling = child;
        }
    }

    struct NodeStack {
        Node** items;
        size_t size;
        size_t capacity;
    };

    static bool stackPush(NodeStack& stack, Node* node) {
        if (!node) return true;
        if (stack.size >= stack.capacity) {
            size_t newCap = stack.capacity == 0 ? 32 : stack.capacity * 2;
            Node** newItems = (Node**)malloc(newCap * sizeof(Node*));
            if (!newItems) {
                return false;
            }
            if (stack.items && stack.size > 0) {
                memcpy(newItems, stack.items, stack.size * sizeof(Node*));
            }
            free(stack.items);
            stack.items = newItems;
            stack.capacity = newCap;
        }
        stack.items[stack.size++] = node;
        return true;
    }

    static Node* stackPop(NodeStack& stack) {
        if (stack.size == 0) return nullptr;
        return stack.items[--stack.size];
    }

    /* Iteratively frees the entire tree to keep stack usage low. */
    static void freeTree(Node* node) {
        NodeStack stack{nullptr, 0, 0};
        if (!stackPush(stack, node)) {
            return;
        }
        while (stack.size > 0) {
            Node* currentNode = stackPop(stack);
            if (currentNode->firstChild) stackPush(stack, currentNode->firstChild);
            if (currentNode->nextSibling) stackPush(stack, currentNode->nextSibling);
            delete currentNode;
        }
        free(stack.items);
    }

    void printTreeIterative() const {
        struct Entry {
            const Node* node;
            int depth;
        };

        Entry* items = nullptr;
        size_t size = 0;
        size_t capacity = 0;

        auto push = [&](const Node* node, int depth) -> bool {
            if (!node) return true;
            if (size >= capacity) {
                size_t newCap = capacity == 0 ? 32 : capacity * 2;
                Entry* newItems = (Entry*)malloc(newCap * sizeof(Entry));
                if (!newItems) {
                    return false;
                }
                if (items && size > 0) {
                    memcpy(newItems, items, size * sizeof(Entry));
                }
                free(items);
                items = newItems;
                capacity = newCap;
            }
            items[size++] = {node, depth};
            return true;
        };

        if (!push(root, 0)) {
            free(items);
            return;
        }
        while (size > 0) {
            Entry entry = items[--size];
            const Node* node = entry.node;
            int depth = entry.depth;

            for (int i = 0; i < depth; ++i) {
                printf("  ");
            }
            if (node->isTag) {
                printf("Tag: %s", htmlTagToString(node->tag));
                for (int i = 0; i < node->attr_count; ++i) {
                    printf(" [%s=\"%s\"]", node->attributes[i].name, node->attributes[i].value);
                }
                printf("\n");
            } else {
                printf("Content: %s\n", node->data);
            }

            /* Push sibling first so child is processed next (preserves original traversal order). */
            push(node->nextSibling, depth);
            push(node->firstChild, depth + 1);
        }

        free(items);
    }

    const char* extractTagName(const char* start, char* buffer, bool& copiedFully) {
        const char* ptr = start;
        while (*ptr != ' ' && *ptr != '>' && *ptr != '\0') {
            ptr++;
        }
        const size_t length = (size_t)(ptr - start);
        copiedFully = copyToBuffer(start, length, buffer, MAX_TAG_NAME_LENGTH);
        if (!copiedFully) {
            printf("Tag name too long\n");
        }
        return ptr;
    }

    int parseAttributes(const char* start, const char* end, Node* node) {
        const char* ptr = start;
        while (ptr < end && *ptr != '>') {
            while (ptr < end && isspace(*ptr)) {
                ptr++;
            }

            if (ptr >= end || *ptr == '>') {
                break;
            }

            const char* attrNameStart = ptr;
            while (ptr < end && *ptr != '=' && !isspace(*ptr) && *ptr != '\0') {
                ptr++;
            }
            char attrName[MAX_ATTR_NAME_LENGTH] = {0};
            const size_t attrNameLength = (size_t)(ptr - attrNameStart);
            if (!copyToBuffer(attrNameStart, attrNameLength, attrName, MAX_ATTR_NAME_LENGTH)) {
                printf("Attribute name too long\n");
                return -1;
            }

            if (ptr >= end || *ptr != '=' || *ptr == '\0') return -1;
            ptr++;

            if (ptr >= end || *ptr != '\"') return -1;
            ptr++;

            const char* attrValueStart = ptr;
            while (ptr < end && *ptr != '\"' && *ptr != '\0') {
                ptr++;
            }
            if (ptr >= end || *ptr != '\"') return -1;
            char attrValue[MAX_ATTR_VALUE_LENGTH] = {0};
            const size_t attrValueLength = (size_t)(ptr - attrValueStart);
            if (!copyToBuffer(attrValueStart, attrValueLength, attrValue, MAX_ATTR_VALUE_LENGTH)) {
                printf("Attribute value too long\n");
                return -1;
            }
            ptr++;

            if (!node->addAttribute(attrName, attrValue)) {
                return -1;
            }
        }
        return 0;
    }
};

#endif /* RETROS_BROWSER_HTML_HPP */
