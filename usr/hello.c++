#include <lib/printf.h>

class Object {  
public:  
	Object();

	~Object() {
		printf("Deconstruct object\n");
	}

	void insert(int i)    
	{    
		id = i;     
	}    
	void display()    
	{    
		print("Display test\n");    
	}
private:
	int id;
};

Object::Object() {
		printf("Construct object\n");
	}

int main(void) {  
    Object s1; 
    Object s2;
    s1.insert(201);    
    s2.insert(202);    
    s1.display();    
    s2.display();

	printf("Done\n");
	return 0;
}