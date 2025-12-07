#include <libc.h>
#include <lib/printf.h>
#include <lib/syscall.h>

static const size_t MAX_TAG_NAME_LENGTH = 10;
static const size_t MAX_ATTR_NAME_LENGTH = 15;
static const size_t MAX_ATTR_VALUE_LENGTH = 50;
static const size_t MAX_ATTRIBUTES = 5;
static const size_t MAX_NODE_DATA_LENGTH = 100;

/* Enumeration for HTML tags */
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
    A, Div, H1, H2, H3, H4, H5, H6
};

struct TagMapping {
    HTMLTag tag;
    const char* name;
};

constexpr TagMapping kTagMappings[] = {
    {Html, "html"}, {Body, "body"}, {P, "p"}, {Input, "input"},
    {Checkbox, "checkbox"}, {Button, "button"},
    {Label, "label"}, {Layout, "layout"}, {Spacing, "spacing"},
    {A, "a"}, {Div, "div"}, {H1, "h1"}, {H2, "h2"},
    {H3, "h3"}, {H4, "h4"}, {H5, "h5"}, {H6, "h6"},
};

bool stringsEqual(const char* a, const char* b) {
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

bool copyToBuffer(const char* source, size_t length, char* destination, size_t destinationSize) {
    if (destinationSize == 0 || destination == nullptr || source == nullptr) {
        return false;
    }

    const int copyLength = (int)(length < destinationSize - 1 ? length : destinationSize - 1);
    memcpy(destination, source, copyLength);
    destination[copyLength] = '\0';
    return (size_t)copyLength == length;
}

HTMLTag stringToHTMLTag(const char* str) {
    for (const auto& mapping : kTagMappings) {
        if (stringsEqual(str, mapping.name)) {
            return mapping.tag;
        }
    }
    return Unknown;
}

const char* htmlTagToString(HTMLTag tag) {
    for (const auto& mapping : kTagMappings) {
        if (mapping.tag == tag) {
            return mapping.name;
        }
    }
    return "unknown";
}

/* Structure for HTML attributes */
struct HTMLAttribute {
    char name[MAX_ATTR_NAME_LENGTH];
    char value[MAX_ATTR_VALUE_LENGTH];
};

/* Structure for HTML nodes */
struct Node {
    HTMLTag tag;

    HTMLAttribute attributes[MAX_ATTRIBUTES];
    int attr_count;

    char data[MAX_NODE_DATA_LENGTH];

    bool isTag;

    Node* parent;
    Node* firstChild;
    Node* nextSibling;

    Node(HTMLTag tag, const char* data, bool isTagNode, Node* parentNode) : tag(tag), attr_count(0), isTag(isTagNode), parent(parentNode), firstChild(nullptr), nextSibling(nullptr) {
        copyToBuffer(data, (size_t)strlen(data), this->data, MAX_NODE_DATA_LENGTH);
    }

    ~Node() {
        delete firstChild;
        delete nextSibling;
    }

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
    HTMLParser(const char* htmlContent) : html(htmlContent), root(new Node(Unknown, "root", true, nullptr)), current(root) {

    }

    ~HTMLParser() {
        delete root;
    }

    int parse() {
        const char* pos = html;
        while (*pos != '\0') {
            pos = skipWhitespace(pos);
            if (*pos == '<') {
                if (*(pos + 1) != '/') {
                    
                    /* Start of a new tag */
                    const char* tagStart = pos + 1;
                    pos = findTagEnd(pos);
                    if (*pos == '\0') {
                        printf("Malformed HTML: Tag not closed\n");
                        return -1; /* Error: Malformed HTML */
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
                        return -2; /* Error parsing attributes */
                    }
                    
                    addChild(child);
                    current = child;
                } else {
                    /* End of a current tag */
                    pos = findTagEnd(pos);
                    if (*pos == '\0') return -1;
                    current = current->parent ? current->parent : current;
                }
            } else {
                /* Text content */
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
        return 0; /* Success */
    }

    void printTree() const {
        printTreeRecursive(root, 0);
    }

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

    bool isspace(char c) const {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

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

    void printTreeRecursive(const Node* node, int depth) const {
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
        if (node->firstChild) {
            printTreeRecursive(node->firstChild, depth + 1);
        }
        if (node->nextSibling) {
            printTreeRecursive(node->nextSibling, depth);
        }
    }


    /* Utility function to extract the tag name from a given position */
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
        return ptr; /* Return the position after the tag name */
    }

    /* Utility function to parse attributes within a tag */
    int parseAttributes(const char* start, const char* end, Node* node) {
        const char* ptr = start;
        while (ptr < end && *ptr != '>') {
            /* Skip any leading whitespace */
            while (ptr < end && isspace(*ptr)) {
                ptr++;
            }

            if (ptr >= end || *ptr == '>') {
                break;
            }

            /* Extract attribute name */
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

            /* Check for '=' after attribute name */
            if (ptr >= end || *ptr != '=' || *ptr == '\0') return -1;
            ptr++; /* Skip '=' */

            /* Check for opening quote of attribute value */
            if (ptr >= end || *ptr != '\"') return -1;
            ptr++; /* Skip opening quote */

            /* Extract attribute value */
            const char* attrValueStart = ptr;
            while (ptr < end && *ptr != '\"' && *ptr != '\0') {
                ptr++;
            }
            if (ptr >= end || *ptr != '\"') return -1; /* Missing closing quote */
            char attrValue[MAX_ATTR_VALUE_LENGTH] = {0};
            const size_t attrValueLength = (size_t)(ptr - attrValueStart);
            if (!copyToBuffer(attrValueStart, attrValueLength, attrValue, MAX_ATTR_VALUE_LENGTH)) {
                printf("Attribute value too long\n");
                return -1;
            }
            ptr++; /* Skip closing quote */

            /* Add attribute to node */
            if (!node->addAttribute(attrName, attrValue)) {
                return -1;
            }
        }
        return 0;
    }
};

int main() {
    int ret;
    const char* htmlContent = 
    "<html lang=\"en\">"
    "<body>"
    "    <div class=\"container\">"
    "        <h1 id=\"header\">Welcome to the Test Page</h1>"
    "        <p class=\"text-muted\">This is a paragraph with <a href=\"https://example.com\" target=\"_blank\">a link</a>.</p>"
    "        <input type=\"text\" placeholder=\"Enter Text\" name=\"inputField\"></input>"
    "        <button type=\"submit\">Submit</button>"
    "    </div>"
    "</body>"
    "</html>\0";

    HTMLParser parser(htmlContent);
    ret = parser.parse();
    if (ret != 0) {
        printf("Error: %s\n", HTMLParser::getHtmlError(ret));
    }
    parser.printTree();

    return 0;
}
