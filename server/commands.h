enum MESSAGE_TYPE {
  MESSAGE_TYPE_QUIT = 1,
  MESSAGE_TYPE_PING,
  MESSAGE_TYPE_NICKNAME,
  MESSAGE_TYPE_JOIN,
  MESSAGE_TYPE_KICK,
  MESSAGE_TYPE_MUTE,
  MESSAGE_TYPE_UNMUTE,
  MESSAGE_TYPE_WHOIS,
  MESSAGE_TYPE_HELP,
  MESSAGE_TYPE_RETRANSMISSION,
};

int identify_message_type(char *message);