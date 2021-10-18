FROM ubuntu

RUN apt update -y && apt upgrade -y

RUN apt install -y nginx vim curl

COPY run.sh run.sh
COPY webserv2 /webserv
# COPY favicon.ico /var/www/html/favicon.ico
# COPY complex /var/www/html/complex
# COPY push_swap.pdf /var/www/html/push_swap.pdf

# RUN chmod 7777 run.sh

# EXPOSE 80

ENTRYPOINT ["sh","run.sh" ]