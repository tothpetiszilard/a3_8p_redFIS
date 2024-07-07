# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES "can")
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "vwtp.c" "vwtp_cfg.c" )
set(COMPONENT_ADD_INCLUDEDIRS "../can" "../dashapp" "../kwp2000" "../navapp")

register_component()