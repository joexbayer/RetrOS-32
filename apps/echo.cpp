#include <lib/graphics.h>
#include <util.h>
#include <colors.h>
#include <lib/net.h>
#include "../apps/utils/cppUtils.hpp"


class TcpEcho : public Window {
public:
    TcpEcho(int width, int height) : Window(200, 200, "Echo", 0) {
        this->width = width;
        this->height = height;

        drawRect(0, 0, width, height, COLOR_VGA_BG);
    }


    void Run(){

        TcpClient client("tcpbin.com", 4242);
        struct gfx_event e;

        char buf[100] = {0};
        char recvBuf[100] = {0};
        int i = 0;

        while (1){

            gfx_get_event(&e, GFX_EVENT_BLOCKING);
            switch (e.event)
            {
            case GFX_EVENT_RESOLUTION:
                setSize(e.data, e.data2);
                break;
            case GFX_EVENT_EXIT:
                Cleanup();
                exit();
                return ;
            case GFX_EVENT_KEYBOARD:
                switch (e.data)
                {
                case '\b':
                    if(i > 0){
                        i--;
                        buf[i] = 0;
                        drawFormatText(0, 0, COLOR_VGA_FG, buf);
                    }
                break;
                case '\n':
                    buf[i++] = '\n';
                    client.sendMsg(buf);   
                    i = 0;

                    client.recvMsg(recvBuf, 100);

                    drawFormatText(0, 10, COLOR_VGA_FG, recvBuf);
                    break;
                default:
                    buf[i++] = e.data;
                    drawFormatText(0, 0, COLOR_VGA_FG, buf);
                    break;
                }
                break;
            default:
                break;
            }
        }
    }

    void Cleanup(){
        
    }

    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
    }

private:
    int sd;
    int width, height;
};

int main()
{
    TcpEcho demo(200, 200);
    demo.setHeader("tcpbin.com");
    demo.Run();

    while (1)
    {
        /* code */
    }
    
    

    return 0;
}
