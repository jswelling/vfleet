class rgbImage;

class baseImageHandler { 
public:
  baseImageHandler();
  virtual ~baseImageHandler();
  virtual display( rgbImage image );
};

class netImageHandler { 
public:
  netImageHandler( char *component_in, int instance_in );
  ~netImageHandler();
  virtual display( rgbImage image );
private:
  char *component;
  int instance;
};

/*
class XImageHandler {
public:
  XImageHandler( Display *dpy_in, int screen_in, Window win_in );
  ~XImageHandler();
  virtual display( rgbImage image );
private:
  Display *dpy;
  int screen;
  Window win;
}; 
*/
