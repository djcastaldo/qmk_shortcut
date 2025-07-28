LTO_ENABLE = yes
CONSOLE_ENABLE = no
DEFERRED_EXEC_ENABLE = yes
TAP_DANCE_ENABLE = yes
CAPS_WORD_ENABLE = yes
DYNAMIC_MACRO_ENABLE = yes
KEY_LOCK_ENABLE = yes
LEADER_ENABLE = yes
MOUSEKEY_ENABLE = yes
SRC += features/layer_lock.c
ifneq ("$(wildcard keyboards/shortcut/bridge75/keymaps/djcastaldo/secrets.c)","")
  SRC += secrets.c
endif
ifeq ($(strip $(NO_SECRETS)), yes)
    OPT_DEFS += -DNO_SECRETS
endif
