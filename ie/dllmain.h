// dllmain.h : Declaration of module class.

class CesteidpluginieModule : public ATL::CAtlDllModuleT< CesteidpluginieModule > 
{
public :
	DECLARE_LIBID(LIBID_esteidpluginieLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ESTEIDPLUGINIE, "{94F9C9B6-1636-4CFD-B4CC-507DFBA963A5}")
};

extern class CesteidpluginieModule _AtlModule;
