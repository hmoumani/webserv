FROM debian:buster

RUN apt update && apt upgrade

RUN apt install -y nginx curl

COPY run.sh run.sh

RUN chmod 7777 run.sh

EXPOSE 80

ENTRYPOINT ["sh","run.sh" ]