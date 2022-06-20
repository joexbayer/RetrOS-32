#
# Because compilation isnt consitent, even across different linux distribution.
# I was forced to compile in a consitent docker environment.
# Ubunut with build-essentials works.
#

FROM ubuntu
RUN apt-get update && \
  apt-get install -y build-essential

WORKDIR /NETOS
CMD ["make"]