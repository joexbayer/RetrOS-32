#include <iostream>
#include <cstring>

#define MAX_TAG_NAME_LENGTH 10
#define MAX_ATTR_NAME_LENGTH 15
#define MAX_ATTR_VALUE_LENGTH 50
#define MAX_ATTRIBUTES 5

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
    Spacing
};

HTMLTag stringToHTMLTag(const char* str) {
    if (strncmp(str, "html", 4) == 0) return Html;
    if (strncmp(str, "body", 4) == 0) return Body;
    if (strncmp(str, "p", 1) == 0) return P;
    if (strncmp(str, "input", 5) == 0) return Input;
    if (strncmp(str, "checkbox", 8) == 0) return Checkbox;
    if (strncmp(str, "button", 6) == 0) return Button;
    if (strncmp(str, "label", 5) == 0) return Label;
    if (strncmp(str, "layout", 6) == 0) return Layout;
    if (strncmp(str, "spacing", 7) == 0) return Spacing;
    return Unknown;
}

const char* htmlTagToString(HTMLTag tag) {
    switch (tag) {
    case Html:
        return "html";
    case Body:
        return "body";
    case P:
        return "p";
    case Input:
        return "input";
    case Checkbox:
        return "checkbox";
    case Button:
        return "button";
    case Label:
        return "label";
    case Layout:
        return "layout";
    case Spacing:
        return "spacing";
    default:
        return "unknown";
    }
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

    char data[100];
    
    bool isTag;
    
    Node* parent;
    Node* firstChild;
    Node* nextSibling;

    Node(HTMLTag tag, const char* data, bool isTagNode, Node* parentNode)
        : tag(tag), attr_count(0), isTag(isTagNode), parent(parentNode), firstChild(nullptr), nextSibling(nullptr) {
        strncpy(this->data, data, 99);
        this->data[99] = '\0';
    }

    ~Node() {
        delete firstChild;
        delete nextSibling;
    }

    void addAttribute(const char* name, const char* value) {
        if (attr_count < MAX_ATTRIBUTES) {

            strncpy(attributes[attr_count].name, name, MAX_ATTR_NAME_LENGTH - 1);
            attributes[attr_count].name[MAX_ATTR_NAME_LENGTH - 1] = '\0';

            strncpy(attributes[attr_count].value, value, MAX_ATTR_VALUE_LENGTH - 1);
            attributes[attr_count].value[MAX_ATTR_VALUE_LENGTH - 1] = '\0';

            attr_count++;
        }
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
                    if (*pos == '\0') return -1; /* Error: Malformed HTML */

                    char tagStr[MAX_TAG_NAME_LENGTH] = {0};
                    const char* tagEnd = extractTagName(tagStart, tagStr);
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
                    char content[100] = {0};
                    strncpy(content, contentStart, pos - contentStart < 99 ? pos - contentStart : 99);
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
            std::cout << "  ";
        }
        if (node->isTag) {
            std::cout << "Tag: " << htmlTagToString(node->tag);
            /* Print attributes if any */
            for (int i = 0; i < node->attr_count; ++i) {
                std::cout << " [" << node->attributes[i].name << "=\"" << node->attributes[i].value << "\"]";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Content: " << node->data << std::endl;
        }
        if (node->firstChild) {
            printTreeRecursive(node->firstChild, depth + 1);
        }
        if (node->nextSibling) {
            printTreeRecursive(node->nextSibling, depth);
        }
    }


    /* Utility function to extract the tag name from a given position */
    const char* extractTagName(const char* start, char* buffer) {
        const char* ptr = start;
        while (*ptr != ' ' && *ptr != '>' && *ptr != '\0') {
            *buffer++ = *ptr++;
        }
        *buffer = '\0'; /* Null-terminate the string */
        return ptr; /* Return the position after the tag name */
    }

    /* Utility function to parse attributes within a tag */
    int parseAttributes(const char* start, const char* end, Node* node) {
        const char* ptr = start;
        while (ptr < end && *ptr != '>') {
            /* Skip any leading whitespace */
            while (isspace(*ptr)) ptr++;

            /* Extract attribute name */
            const char* attrNameStart = ptr;
            while (*ptr != '=' && !isspace(*ptr) && *ptr != '\0' && ptr < end) {
                ptr++;
            }
            char attrName[MAX_ATTR_NAME_LENGTH] = {0};
            strncpy(attrName, attrNameStart, ptr - attrNameStart);

            /* Check for '=' after attribute name */
            if (*ptr != '=' || *ptr == '\0') return -1;
            ptr++; /* Skip '=' */

            /* Check for opening quote of attribute value */
            if (*ptr != '\"') return -1;
            ptr++; /* Skip opening quote */

            /* Extract attribute value */
            const char* attrValueStart = ptr;
            while (*ptr != '\"' && *ptr != '\0' && ptr < end) {
                ptr++;
            }
            if (*ptr != '\"') return -1; /* Missing closing quote */
            char attrValue[MAX_ATTR_VALUE_LENGTH] = {0};
            strncpy(attrValue, attrValueStart, ptr - attrValueStart);
            ptr++; /* Skip closing quote */

            /* Add attribute to node */
            node->addAttribute(attrName, attrValue);
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
        std::cout << "Error: " << HTMLParser::getHtmlError(ret) << std::endl;
    }
    parser.printTree();

    return 0;
}
