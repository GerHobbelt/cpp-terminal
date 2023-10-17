//
// part of Demo 
//

#pragma once

#if defined(BUILD_MONOLITHIC)

#ifdef __cplusplus
extern "C" {
#endif

	int cppterminal_args_example_main(void);
	int cppterminal_attach_console_example_main(void);
	int cppterminal_attach_console_minimal_example_main(void);
	int cppterminal_cin_cooked_example_main(void);
	int cppterminal_cin_raw_example_main(void);
	int cppterminal_colors_example_main(void);
	int cppterminal_cout_example_main(void);
	int cppterminal_events_example_main(void);
	int cppterminal_keys_example_main(void);
	int cppterminal_kilo_example_main(int argc, const char** argv);
	int cppterminal_menu_example_main(void);
	int cppterminal_menu_window_example_main(void);
	int cppterminal_minimal_example_main(void);
	int cppterminal_prompt_immediate_example_main(void);
	int cppterminal_prompt_multiline_example_main(void);
	int cppterminal_prompt_not_immediate_example_main(void);
	int cppterminal_prompt_simple_example_main(void);
	int cppterminal_slow_events_example_main(void);
	int cppterminal_styles_example_main(void);
	int cppterminal_utf8_example_main(void);

#ifdef __cplusplus
}
#endif

#endif
