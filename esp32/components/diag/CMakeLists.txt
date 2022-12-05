# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES "kwp2000" )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "enginediag.c")
set(COMPONENT_ADD_INCLUDEDIRS  "../kwp2000" )

register_component()