#include <lib/graphics.h>
#include <util.h>
#include <colors.h>
#include <lib/net.h>

static unsigned short htons(unsigned short data)
{
  return (((data & 0x00ff) << 8) |
           (data & 0xff00) >> 8);
}

static unsigned int htonl(unsigned int data)
{
  return (((data & 0x000000ff) << 24) |
          ((data & 0x0000ff00) << 8)  |
          ((data & 0x00ff0000) >> 8)  |
          ((data & 0xff000000) >> 24) );
}


class TcpEcho : public Window {
public:
    TcpEcho(int width, int height) : Window(200, 200, "Echo", 0) {
        this->width = width;
        this->height = height;

        client = new TcpClient("tcpbin.com", 4242);

        drawRect(0, 0, width, height, COLOR_VGA_BG);
    }


    void Run(){
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
                    client->sendMsg(buf);   
                    i = 0;

                    client->recvMsg(recvBuf, 100);

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
        delete client;
    }

    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
    }

private:
    int sd;
    int width, height;
    TcpClient* client;

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
