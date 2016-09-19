# Makefile voor C-library's
# Maakt 1 model aan. Aanroepen via batch voor elk model

# Volgende macros bevatten de environment

#implicit rule voor object-files:
.c.obj:
	tcc -c -m$(MDL) $*
	tlib $(LIB)$(MDL) -+$* 
	del $*.OBJ

#dependencies library:
$(LIB).lib: 	TB_BTREE.OBJ TB_CACHE.OBJ TB_FILES.OBJ TB_FLDAT.OBJ \
		TB_MENU.OBJ TB_POPM.OBJ TB_PRN.OBJ TB_RCDIO.OBJ \
		TB_SCRN.OBJ TB_SORT.OBJ TB_STATL.OBJ TB_SUBS.OBJ \
		TB_TERM.OBJ TB_WINS.OBJ

