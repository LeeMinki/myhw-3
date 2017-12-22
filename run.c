#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <stdio.h>
#include "run.h"
#include "util.h"

void *top;
void *cur;
void *base;

p_meta find_meta(p_meta *cur, size_t size) 
{
	p_meta index = base;
	p_meta result = 0;

  // empty
	if(cur == base) 
    return 0;

	switch(fit_flag)
  {
		case FIRST_FIT:
		{
			while(index)
      {
				if(index->free != 0 && index->size >= size)
        {
					result = index;
					break;
				}
        index = (index->next);
			}
		}
		break;

		case BEST_FIT:
    {
      p_meta b = NULL;
      int min = 2e9;
      while(index)
      {
        if(index->free == 1 && index->size >= size && index->size < min)
        {
          b = index;
          min = index->size;
        }
        index = index->next;
      }
      result = b; 
    }
    break;
    

    case WORST_FIT:
    {
      p_meta w = NULL;
      int max = 0;
      while(index)
      {
        if(index->free == 1 && index->size >=size && index->size > max)
        {     
          w = index;
          max = index->size;
        }    
        index = index->next;
      }
      result = w;
    }
    break;
  }
	return result;
}

void *m_malloc(size_t size) 
{
  // 32bit 32bit 32bit
  if(size%4 != 0)
    size = ((size/4)+1)*4;
  // first experience -> initialize
	if(base == 0)
		base = top = cur = sbrk(0);
	// return 0 or else
	p_meta check = find_meta(top, size);

	// There is no block
	if(check == 0)
  {
    sbrk(top+size+META_SIZE);
    p_meta new = top;
		p_meta now = cur;

    // first experience
		if(top == cur)
      new->prev = 0;
    // experienced
		else
    {
      now->next = new;
			new->prev = cur;
    }
    // set linked list
		new->free = 0;
		new->next = 0;
		new->size = size;
		cur = new;

		top += size + META_SIZE;
    return new->data;
	}
  // Block exists
  else
  {
    m_realloc(check->data, size);
		return check->data;
	}
}

void m_free(void *ptr) 
{
	p_meta delete = ptr - META_SIZE;
  delete->free = 1;

	if(delete->prev != NULL)
  {
    if(delete->prev->free==1)
    {
		  delete->prev->size += delete->size+META_SIZE;
      delete->prev->next = delete->next;
      delete->next->prev = delete->prev;
    }
  }
  if(delete->next != NULL)
  {
    if(delete->next->free==1)
    {
		  delete->size += delete->next->size+META_SIZE;
      delete->next->next->prev = delete;
      delete->next = delete->next->next;
    }
  }
  else if(delete->prev != NULL)
    (delete->prev)->next = NULL;
}

void* m_realloc(void* ptr, size_t size)
{
	p_meta reborn = ptr - META_SIZE;
	// 32bit 32bit 32bit
  if(size%4 != 0)
    size = ((size/4)+1)*4;
	reborn->free = 0;

  // coalsec next wit the target block.
	if((reborn->next) != NULL)
  {
		if((reborn->next)->free == 1)
    {
      reborn->size += (reborn->next)->size + META_SIZE;
			reborn->next = (reborn->next)->next;
		}
  }
  // size is small...
	if((reborn->size) < size)
  {
    // can't use reborn, bring another things.
		reborn->free = 1;
    // make new one
		p_meta born = m_malloc(size);
		memcpy(born, ptr, size);
		return born;
	}
  // assign new one in reborn block
	else if(reborn->size > size)
  {
		if(reborn->size > size + META_SIZE)
    {
			p_meta new = ptr + size;
      new->free = 1;
      new->size = (reborn->size) - size - META_SIZE;
			new->prev = reborn;
			new->next = reborn->next;
      reborn->size = size;
      reborn->next = new;
		}
	}
}
