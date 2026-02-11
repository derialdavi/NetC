CC=gcc
CFLAGS=-Wall -Wextra -Werror -O2 -std=c2x -D_GNU_SOURCE
DEBFLAGS=-g

SRCDIR=./src
OBJDIR=./obj
LIBDIR=$(shell gcc -print-file-name=libc.so | xargs dirname)

LIBNAME=netc

SRCS=$(wildcard $(SRCDIR)/*.c)
HEADERS=$(notdir $(wildcard $(SRCDIR)/*.h))
OBJS=$(SRCS:.c=.o)
TARGET=lib$(LIBNAME).so

.PHONY: build clean install uninstall test

build: $(TARGET)

clean:
	rm -rf $(OBJDIR)/*

install: $(TARGET)
	install -m 755 $(OBJDIR)/$(TARGET) $(LIBDIR)
	install -m 644 $(SRCDIR)/*.h /usr/include

uninstall:
	rm -f $(LIBDIR)/$(TARGET) $(addprefix /usr/include/,$(HEADERS))

$(TARGET): $(OBJS)
	$(CC) -shared -o $(OBJDIR)/$@ $(addprefix $(OBJDIR)/,$(notdir $^))

%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $(OBJDIR)/$(notdir $@)
