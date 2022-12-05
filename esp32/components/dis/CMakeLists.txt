# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES "dashapp" "stalkButtons" "diag")
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "dis.c" "dis_cfg.c" "dis_decode.c")
set(COMPONENT_ADD_INCLUDEDIRS  "../dashapp" "../diag" "../stalkButtons")

register_component()