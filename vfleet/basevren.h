class baseLogger;
class baseImageHandler;
class rgbImage;
class VRenServer;

class baseVRen {
public:
  baseVRen( const int xin, const int yin, const int zin,
	   baseLogger *logger_in, baseImageHandler *imagehandler, 
	   void (*error_handler)(int error_id, baseVRen *renderer), 
	   void (*fatal_handler)(int error_id, baseVRen *renderer) );
  baseVRen( const int xin, const int yin, const int zin,
	    baseLogger *logger_in, baseImageHandler *imagehandler, 
	    baseVRen *owner_in );
  virtual ~baseVRen(); 
  virtual boolean SetVolume( int fdesc );
  virtual boolean SetParamBlock( NetVParms *nvp );
  virtual boolean StartRender();
  virtual boolean AbortRender();
  virtual void error( int error_id );
  virtual void fatal( int error_id );
protected:
  int xdim;
  int ydim;
  int zdim;
  baseLogger *logger;
  baseImageHandler *ihandler;
  rgbImage *image;
  void (*error_proc)(int error_id, baseVRen *renderer);
  void (*fatal_proc)(int error_id, baseVRen *renderer);
  baseVRen *owner;
};

class raycastVRen: public baseVRen {
public:
  raycastVRen( const int xin, const int yin, const int zin,
	       baseLogger *logger, baseImageHandler *imagehandler, 
	       void (*error_handler)(int error_id, baseVRen *renderer), 
	       void (*fatal_handler)(int error_id, baseVRen *renderer) );
  raycastVRen( const int xin, const int yin, const int zin,
	       baseLogger *logger, baseImageHandler *imagehandler, 
	       baseVRen *owner );
  ~raycastVRen(); 
  boolean StartRender();
};

class volcompVRen: public baseVRen {
public:
  volcompVRen( const int xin, const int yin, const int zin,
	       baseLogger *logger, baseImageHandler *imagehandler, 
	       void (*error_handler)(int error_id, baseVRen *renderer), 
	       void (*fatal_handler)(int error_id, baseVRen *renderer) );
  volcompVRen( const int xin, const int yin, const int zin,
	       baseLogger *logger, baseImageHandler *imagehandler, 
	       baseVRen *owner );
  ~volcompVRen(); 
};

// Reference info for netVRen
const int vren_param_buf_size= 64;

class netVRen: public baseNet, public baseVRen {
friend class VRenServer;
public:
  netVRen( RemObjInfo *rem_in, const int xin, const int yin, const int zin, 
	   const int type );
  netVRen( const int xin, const int yin, const int zin, const int type,
	   baseLogger *logger_in, baseImageHandler *imagehandler, 
	   void (*error_handler)(int error_id, baseVRen *renderer), 
	   void (*fatal_handler)(int error_id, baseVRen *renderer) );
  netVRen( const int xin, const int yin, const int zin, const int type,
	   baseLogger *logger_in, baseImageHandler *imagehandler, 
	   baseVRen *owner );
  ~netVRen();
  boolean StartRender();
  boolean AbortRender();
  void netrcv(NetMsgType msg);
  static void initialize( const char *name );
  static char *pack_param_info( const int xin, const int yin, const int zin, 
				const int type);
  static void unpack_param_info( char *info, int &xin, int &yin, int &zin,
				 int &type );
  void error( int error_id );
  void fatal( int error_id );
protected:
  static int initialized;
  static void handle_netvren_request();
  static char param_info_buf[ vren_param_buf_size ];
  baseVRen *out_vren;
  int created_own_logger;
};

// Reference info for VRenServers
// Name is lower case because it must match info in command lines
const char VRenServerExeName[]="vrenserver";

// This server produces netVRens
class VRenServer : public baseServer, public baseNet {
public:
  VRenServer();
  VRenServer( RemObjInfo *rem_in );
  ~VRenServer();
  void netrcv(NetMsgType msg);
  static void initialize( const char *cmd );
  void create_vren( RemObjInfo *rem_in, int xin, int yin, int zin, int type );
protected:
  static int initialized;
  static void handle_server_request();
  baseLogger *logger;
};

