# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES "vwtp")
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "dashapp.c" )
set(COMPONENT_ADD_INCLUDEDIRS "../vwtp")

register_component()