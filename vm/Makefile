
SDL_CFLAGS := `sdl2-config --cflags`
SDL_LIBS   := `sdl2-config --libs`

CPPFLAGS += -MMD -Wall -g $(SDL_CFLAGS)

OBJS = host_sdl2.o can.o img.o ini.o main.o pan.o random.o \
	sc_asset.o sc_console.o sc_debug.o sc_file.o sc_image.o sc_input.o sc_sound.o sc_sprite.o sc_string.o sc_system.o sc_time.o sc_window.o \
	sob.o util.o \
	vm.o vm_array.o vm_object.o vm_opcodes.o vm_stack.o vm_thread.o
DEPS = $(OBJS:.o=.d)

vm: $(OBJS)
	$(CC) -o $@ $^ $(SDL_LIBS)

clean:
	rm -f *.o *.d

-include $(DEPS)
