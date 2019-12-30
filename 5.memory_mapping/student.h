/*
 * OS Assignment #5
 *
 * @file student.h
 */

#ifndef __STUDENT_H__
#define __STUDENT_H__

#include <sys/types.h> /* for pid_t, size_t */
#include <string.h>    /* for strcmp() */

typedef struct
{
  char  name[64];
  char  studentID[12];
  int   age;
  int   gender; /* 1:male, 0:female */
  char  phone[24];
  char  e_mail[80];
} Student;

#ifndef offsetof
#define offsetof(s,m) (size_t)&(((s *)0)->m)
#endif

static inline const char *
studnet_lookup_attr_with_offset (size_t offset)
{
  switch (offset)
    {
    case offsetof (Student, name):
      return "name";
	case offsetof(Student, studentID):
		return "studentID";
    case offsetof (Student, age):
      return "age";
    case offsetof (Student, gender):
      return "gender";
    case offsetof (Student, phone):
      return "phone";
    case offsetof (Student, e_mail):
      return "e_mail";
    default:
      break;
    }
  return "unknown";
}

static inline size_t 
student_get_offset_of_attr (const char *attr_name)
{
  if (!strcmp (attr_name, "name"))
    return offsetof (Student, name);
  if (!strcmp(attr_name, "studentID"))
	  return offsetof(Student, studentID);
  if (!strcmp (attr_name, "age"))
    return offsetof (Student, age);
  if (!strcmp (attr_name, "gender"))
    return offsetof (Student, gender);
  if (!strcmp (attr_name, "phone"))
    return offsetof (Student, phone);
  if (!strcmp (attr_name, "e_mail"))
    return offsetof (Student, e_mail);

  return 0;
}

static inline int
student_attr_is_integer (const char *attr_name)
{
  if (!strcmp (attr_name, "age") || !strcmp (attr_name, "gender"))
    return 1;

  return 0;
}

#endif /* __STUDENT_H__ */
