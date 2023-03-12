#include <lib/printf.h>
#include <lib/graphics.h>

class Editor {  
public:  
	Editor() {
		x = 0;
		y = 0;
		textBuffer = (char*) malloc(1000);
	}

	~Editor() {
		free(textBuffer);
	}

	void Save();
	void Open();

	void EditorLoop()
	{

	}

private:
	int fd;
	char* textBuffer;
	int x, y;
};

int main(void) {  
    Editor s1;

	printf("Done\n");
	return 0;
}