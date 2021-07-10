FROM debian:buster

RUN apt update && apt upgrade

RUN apt install -y nginx curl

COPY run.sh run.sh
COPY favicon.ico /var/www/html/favicon.ico

RUN chmod 7777 run.sh

EXPOSE 80

ENTRYPOINT ["sh","run.sh" ]