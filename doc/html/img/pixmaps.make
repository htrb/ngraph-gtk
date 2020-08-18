PIXMAP_DIR = ../../../src/gtk/data/pixmaps
SYSTEM_ICON_DIR = /usr/share/icons/Adwaita/scalable

.SUFFIXES: .png .svg
.svg.png:
	inkscape -w 32 -h 32 -o __$@ $<
	convert -background '#dedad7' -flatten -fuzz 60% -fill '#000000' -opaque '#bebebe' __$@ $@
	rm -f __$@

barc.svg:     $(PIXMAP_DIR)/ngraph_arc-symbolic.svg
	cp $< $@

baxispo.svg:  $(PIXMAP_DIR)/ngraph_axispoint-symbolic.svg
	cp $< $@

bcross.svg:   $(PIXMAP_DIR)/ngraph_cross-symbolic.svg
	cp $< $@

bdatapo.svg:  $(PIXMAP_DIR)/ngraph_datapoint-symbolic.svg
	cp $< $@

bdraw.svg:    $(PIXMAP_DIR)/ngraph_draw-symbolic.svg
	cp $< $@

beval.svg:    $(PIXMAP_DIR)/ngraph_eval-symbolic.svg
	cp $< $@

bframe.svg:   $(PIXMAP_DIR)/ngraph_frame-symbolic.svg
	cp $< $@

bgauss.svg:   $(PIXMAP_DIR)/ngraph_gauss-symbolic.svg
	cp $< $@

blgndpo.svg:  $(PIXMAP_DIR)/ngraph_legendpoint-symbolic.svg
	cp $< $@

bline.svg:    $(PIXMAP_DIR)/ngraph_line-symbolic.svg
	cp $< $@

bmark.svg:    $(PIXMAP_DIR)/ngraph_mark-symbolic.svg
	cp $< $@

bpoint.svg:   $(PIXMAP_DIR)/ngraph_point-symbolic.svg
	cp $< $@

brect.svg:    $(PIXMAP_DIR)/ngraph_rect-symbolic.svg
	cp $< $@

bscale.svg:   $(PIXMAP_DIR)/ngraph_scale-symbolic.svg
	cp $< $@

bsection.svg: $(PIXMAP_DIR)/ngraph_section-symbolic.svg
	cp $< $@

bsingle.svg:  $(PIXMAP_DIR)/ngraph_single-symbolic.svg
	cp $< $@

btext.svg:    $(PIXMAP_DIR)/ngraph_text-symbolic.svg
	cp $< $@

btrim.svg:    $(PIXMAP_DIR)/ngraph_trimming-symbolic.svg
	cp $< $@

bzoom.svg:    $(PIXMAP_DIR)/ngraph_zoom-symbolic.svg
	cp $< $@

bmath.svg: $(SYSTEM_ICON_DIR)/apps/accessories-calculator-symbolic.svg
	cp $< $@

bscundo.svg: $(SYSTEM_ICON_DIR)/actions/edit-undo-symbolic.svg
	cp $< $@

bprint.svg: $(SYSTEM_ICON_DIR)/actions/document-print-symbolic.svg
	cp $< $@

bload.svg: $(SYSTEM_ICON_DIR)/actions/document-open-symbolic.svg
	cp $< $@

bsave.svg: $(SYSTEM_ICON_DIR)/actions/document-save-symbolic.svg
	cp $< $@

bdataopn.svg: $(SYSTEM_ICON_DIR)/mimetypes/text-x-generic-symbolic.svg
	cp $< $@
