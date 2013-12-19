#ifndef	__ABCError_incl
# define	__ABCError_incl


namespace bbcvp {
  
/*! \brief error message class
**
**
*/
class Error
{
public:
	Error(const Error &ee);
	Error(const char *msg=0, const char *msg2=0);
	virtual ~Error();
	void SetMsg(const char *msg, const char *msg2=0);
	inline const char *GetMsg() const { return e_msg; }
protected:
	char	*e_msg;
};


/*! \brief warning message class
**
**
*/
class Warning : public Error
{
public:
	Warning(const Warning &w);
	Warning(const char *msg=0, const char *msg2=0);
	virtual ~Warning();
};

  
}

#endif
