#
# Because GCC / LD compilation isnt consistent, even across different linux distributions.
# I was forced to compile in a consistent docker environment.
# Ubunut with build-essentials works.
#

FROM ubuntu
RUN apt-get update && \
  apt-get install -y build-essential && apt-get install gcc-multilib

WORKDIR /NETOS
CMD ["make"]