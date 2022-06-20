void close_client_connection(SOCKET client_socket, fd_set *masterset, CHANNEL **channels);
void transmit_message(SOCKET message_sender, char *message_sender_nickname, char *message, size_t message_length, CHANNEL *channel, int nfds);
void transmit_message_to_channels(SOCKET message_sender, char *message_sender_nickname, char *message, size_t message_length, CHANNEL **channels, int nfds);
