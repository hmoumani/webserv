FROM debian:buster

RUN apt update && apt upgrade

RUN apt install -y nginx curl

COPY run.sh run.sh
COPY favicon.ico /var/www/html/favicon.ico
COPY push_swap.pdf /var/www/html/push_swap.pdf
COPY blessed /var/www/html/blessed


RUN chmod 7777 run.sh

EXPOSE 80

ENTRYPOINT ["sh","run.sh" ]