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
CHANNEL *find_channel(CHANNEL **channels, char *name);
CHANNEL *create_channel(CHANNEL **channels, char *name, SOCKET administrator);
int add_client_to_channel(CHANNEL *channel, SOCKET client);
SOCKET find_client(char **nicknames, char *nickname);
int kick_user(SOCKET administrator, SOCKET client, CHANNEL **channels);
int mute_user(SOCKET administrator, SOCKET client, CHANNEL **channels);
int unmute_user(SOCKET administrator, SOCKET client, CHANNEL **channels);
int whois_user(SOCKET administrator, SOCKET client, char **nicknames);
