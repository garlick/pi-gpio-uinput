DTC = /usr/bin/dtc

ALL = pigc.dtbo

all: $(ALL)


install: $(ALL)
	cp $< /boot/overlays/

clean:
	rm -f $(ALL)

%.dtbo : %-overlay.dts
	$(DTC) -@ -I dts -O dtb -o $@ $<
