CONFIG_MODULE_SIG = n
TARGET_MODULE := fibdrv

obj-m := $(TARGET_MODULE).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) client
	$(MAKE) -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) client out
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

plot:
	gnuplot -p -c execution_time.gp
client: client.c
	$(CC) -o $@ $^

PRINTF = env printf
PASS_COLOR = \e[32;01m
NO_COLOR = \e[0m
pass = $(PRINTF) "$(PASS_COLOR)$1 Passed [-]$(NO_COLOR)\n"

check: all
	$(MAKE) unload
	$(MAKE) load
	sudo ./client > out
	$(MAKE) unload
	@diff -u out scripts/expected.txt && $(call pass)
	@scripts/verify.py

profile: all
	-rm -rf result/
	mkdir -p result
	number=1 ; while [ $$number -le 50 ] ; do \
		$(MAKE) unload ; \
		$(MAKE) load ; \
		REPORT="result/test_$$number.txt" ; \
		sudo taskset -c 1 chrt -f 90 ./client $$REPORT >/dev/null 2>&1 ; \
		$(MAKE) unload ; \
		number=`expr $$number + 1` ; \
	done
	scripts/outlier.py
	gnuplot -p -c confidence_interval.gp

