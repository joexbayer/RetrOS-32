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
    TcpEcho(int width, int height) : Window(200, 200, "TcpEcho", 0) {
        this->width = width;
        this->height = height;

        drawRect(0, 0, width, height, COLOR_VGA_BG);
    }

    void Connect(){
        sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd != 0) {
			exit();
		}

		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = htonl(760180939);
		dest_addr.sin_port = htons(4242);
		dest_addr.sin_family = AF_INET;

        int ret = connect(sd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
		if (ret != 0) {
			exit();
		}
    }

    void Run(){
        Connect();

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
                        gfx_draw_format_text(0, 0, COLOR_VGA_FG, buf);
                    }
                break;
                case '\n':
                    buf[i++] = '\n';
                    send(sd, buf, i, 0);
                    i = 0;
                    recv(sd, recvBuf, 100, 0);
                    gfx_draw_format_text(0, 10, COLOR_VGA_FG, recvBuf);
                    break;
                default:
                    buf[i++] = e.data;
                    gfx_draw_format_text(0, 0, COLOR_VGA_FG, buf);
                    break;
                }
                break;
            default:
                break;
            }
        }
    }

    void Cleanup(){
        close(sd);
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
    demo.setHeader("TcpBin");
    demo.Run();

    while (1)
    {
        /* code */
    }
    
    

    return 0;
}
