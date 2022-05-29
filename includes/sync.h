#ifndef SYNC_H
#define SYNC_H

void spin_lock(int volatile *p);
void spin_unlock(int volatile *p);

#endif