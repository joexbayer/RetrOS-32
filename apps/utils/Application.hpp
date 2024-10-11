#ifndef __APPLICATION_HPP
#define __APPLICATION_HPP

#include <utils/Graphics.hpp>
#include <utils/Thread.hpp>

/**
 * @brief Base class for all applications
 * @details This class is the base class for all applications in the RetrOS
 * operating system. It provides a common interface for all applications to
 * implement.
 */
class Application : public Window {
public:
    /**
     * @brief Constructor for the Application class
     * @details This constructor creates a new application window with the
     * specified width, height, and title.
     * 
     * @param width The width of the application window
     * @param height The height of the application window
     * @param title The title of the application window
     */
    Application(int width, int height, const char* title) : Window(width, height, title, 1) {
    }

    /**
     * @brief Destructor for the Application class
     * @details This destructor cleans up any resources used by the application.
     */
    virtual ~Application() {
    }

    /**
     * @brief Run the application
     * @details This method runs the application. It should be implemented by
     * the derived class to provide the main functionality of the application.
     */
    virtual void run() = 0;

    /**
     * @brief Handle an event
     * @details This method handles an event for the application. It should be
     * implemented by the derived class to handle events such as key presses,
     * mouse clicks, etc.
     * 
     * @param event The event to handle
     */
    virtual void handleEvent(struct gfx_event* event) {
    }

    /**
     * @brief Draw the application window
     * @details This method draws the application window. It should be
     * implemented by the derived class to draw the contents of the window.
     */
    virtual void draw() {
    }

    /**
     * @brief Get the application title
     * @details This method returns the title of the application window.
     * 
     * @return The title of the application window
     */
    const char* getTitle() {
        return mTitle;
    }

    /**
     * @brief Set the application title
     * @details This method sets the title of the application window.
     * 
     * @param title The new title of the application window
     */
    void setTitle(const char* title) {
        mTitle = title;
    }

protected:
    const char* mTitle; /** The title of the application window */

};

/**
 * @brief Application help window
 * @details This class implements a help window for applications. It provides
 * a simple interface for displaying help information to the user.
 */
class ApplicationHelp : public Window {
public:

    ApplicationHelp() : Window(200, 100, "Help", 1) {

    }

    void show(){

    }

};

static void __app_help_entry(void* _){
    ApplicationHelp help;
    help.show();
}

/* Entry code that starts a thread for this window. */
void show_editor_help(bool wait){
    Thread t(__app_help_entry, 0);
    t.start(0);
}



#endif // !__APPLICATION_HPP