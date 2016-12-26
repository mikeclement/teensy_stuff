

/* Recursive descent parser for integer expressions. */

#include  <stdio.h>
#include  <stdint.h>
#include  <string.h>
#include  <ctype.h>
#include  "common.h"
#include  "arm_cm4.h"
#include  "termio.h"
#include  "rdp.h"


#define  MAX_TOKEN_LEN		255

static uint32_t			error;
static char				token[MAX_TOKEN_LEN+1];
static uint32_t			token_type;
static char				*instr;



/*
 *  Local functions
 */
static void				eval_exp1(int32_t *answer);
static void				eval_exp2(int32_t *answer);
static void				eval_exp3(int32_t *answer);
static void				eval_exp4(int32_t *answer);
static void				eval_exp5(int32_t *answer);
static void				eval_exp6(int32_t *answer);
static void				eval_exp7(int32_t *answer);
static void				eval_exp8(int32_t *answer);
static void				atom(int32_t *answer);
static uint32_t			get_token(void);
static int32_t			gethex(char  *str);
static int32_t			getbin(char  *str);
static int32_t			getdec(char  *str);
static void				putback(void);
static void				serror(int32_t errenum);
static char				*rdp_strchr(char  *str, char  c);


/*
 *  rdp      main entry point to the recursive-descent parser
 *
 *  Any routine can call this function to parse the next full algebraic
 *  expression in the null-terminated string passed as argument str.  THe
 *  result of the evaluation, if any, will be written to the variable
 *  pointed to by argument answer.
 *
 *  Upon exit, this routine returns S_OK if parsing was successful,
 *  else it returns an error code.
 *
 */
 
uint32_t  rdp(char  *str, int32_t *answer)
{
	instr = str;					// save start of string globally
	error = RDP_OK;					// assume this works
	token_type = get_token();

	if (token_type == RDP_UNKNOWN)	// if no token found... 
	{
		return  RDP_NO_EXP;			// whine about it
	}
	eval_exp1(answer);				// found a token, parse it
	putback();						// return last token read to input stream
	return  error;
}




/*
 *  eval_exp1      perform comparisons (=, >, <)
 *
 *  This routine handles integer comparisons.  It is only needed if
 *  the parser must handle conditionals, such as IF statements.
 *  I've left it here as source so you can see how the SBasic
 *  compiler processed conditionals.
 *
 *  Note that SBasic used a * suffix to show unsigned comparisons.
 *  For example, an unsigned test of 2 and 3 would be written
 *  "2 >* 3".
 *
 *  Note, also, that I've left in the use of operator tokens
 *  and the operator stack.  These are not used if your parser
 *  is immediately converting a string to an integer, and they
 *  don't appear elsewhere in this file.  But if you are parsing
 *  a string into opcodes for later execution by some kind of
 *  run-time exec, you need the operator stack to control what
 *  execution follows the comparison.
 */

static void  eval_exp1(int32_t  *answer)
{
//	char		c;
//	int			temp;
	
	eval_exp2(answer);				// get 1st argument
#if 0
	switch  (*token)				// based on 1st char in token...*/
	{
		case  '=' :					// if test for equality...
		answer_flag = 0;			// not legal numeric expression
		get_token();				// get 2nd argument
		eval_exp2(&temp);			// parse it
		push_op(OP_TEST);			// push the test opcode
		testtype = OP_EQ;			// save the test type
		return;

		case  '>' :					// if test for gt...
		answer_flag = 0;			// not legal numeric expression
		c = *(token+1);				// get 2nd char
		get_token();				// get 2nd argument
		eval_exp2(&temp);			// parse it
		push_op(OP_TEST);			// push the test opcode
		if (c == '<')				// if "not" modifier...
		{
			testtype = OP_NE;		// show that
			return;
		}
		if (c == '*')				// if unsigned modifier...
		{
			testtype = OP_GTU;		// show that
			return;
		}
		if (c == '=')				// if equal modifier...
		{
			testtype = OP_GE;		// show that
			return;
		}
		testtype = OP_GT;			// just do gt
		return;

		case  '<' :					// if test for lt...
		answer_flag = 0;			// not legal numeric expression
		c = *(token+1);				// get 2nd argument
		get_token();				// get 2nd argument
		eval_exp2(&temp);			// parse it
		push_op(OP_TEST);			// push the test opcode
		if (c == '>')				// if "not" modifier...
		{
			testtype = OP_NE;		// show that
			return;
		}
		if (c == '*')				// if unsigned modifier...
		{
			testtype = OP_LTU;		// show that
			return;
		}
		if (c == '=')				// if equal modifier...
		{
			testtype = OP_LE;		// show that
			return;
		}
		testtype = OP_LT;			// save the test type
		return;
	}
#endif
}




/*
 *  eval_exp2      perform & (AND), | (OR), and ^ (XOR)
 */

 static void  eval_exp2(int32_t  *answer)
 {
	int32_t			   temp;
	register char		op;
	
 	eval_exp3(answer);				// get 1st argument

 	while ((op = *token) == '&' || (op == '|') || (op == '^'))
	{
		get_token();				// get 2nd argument
 		eval_exp3(&temp);			// save parsed value in temp variable

		switch  (op)
		{
			case  '&' :
	 		*answer = *answer & temp;
			break;
 		
			case  '|' :
			*answer = *answer | temp;
			break;
			
	 		case  '^' :
			*answer = *answer ^ temp;
			break;
	 	}
 	}
 }
 
 			
 


/*
 *  eval_exp3      add or subtract two terms
 *
 *  This routine handles all addition or subtraction operations.
 *
 */

static void  eval_exp3(int32_t *answer)
{
	register char	op; 
	int32_t			temp;

	eval_exp4(answer);				// get 1st argument
	while ((op = *token) == '+' || op == '-') 	// while + or - ...
	{
		get_token(); 				// get 2nd argument
		eval_exp4(&temp);			// save parsed value to temp variable
		switch  (op)				// based on operation...
		{
			case '-' :				// if subtraction...
			*answer = *answer - temp;
			break;
			
			case '+':				// if addition...
			*answer = *answer + temp;
			break;
		}
	}
}




/*
 *  eval_exp4      parse the MOD function
 *
 */

static void  eval_exp4(int32_t *answer)
{
	int32_t		temp;

	eval_exp5(answer);						// get 1st argument
	if (*token == '%')						// if this is mod function...
	{
		get_token();						// get 2nd argument
		eval_exp5(&temp);					// save parsed value to temp variable
		*answer = *answer % temp;
	}
}
		


/*
 *  eval_exp5      multiply or divide two factors.
 *
 */

static void  eval_exp5(int32_t *answer)
{
	register char		op; 
	int32_t				temp;

	eval_exp6(answer);						// get first argument
	while ((op = *token) == '*'				// while doing * or /
			|| op == '/')
	{
		get_token();						// get second argument
		eval_exp6(&temp);					// save 2nd argument in temp variable
		switch  (op)						// process operator
		{
			case '*' :						// if doing multiply...
			*answer = *answer * temp;
			break;
			
			case '/':						// if doing divide...
			if (temp != 0)					// and divisor is legal...
			{
				*answer = *answer / temp;
			}
			else 							// bad dovospr
			{
				serror(RDP_DIVIDE_0);
			}
			break;
		}
	}
}



/*
 *  eval_exp6    process integer exponent (**)
 *
 */
 
static void  eval_exp6(int32_t *answer)
{
	int32_t		temp;
	int32_t		i;
	int32_t		t;
	
	eval_exp7(answer);					// evaluate 1st argument
	if ((token[0] == '*') && (token[1] == '*'))		// if doing integer exponentiation
	{
		get_token(); 					// get 2nd argument
		eval_exp7(&temp);				// save 2nd argument to temp variable
		i = *answer;
		for (t = temp-1; t>0; t--)		// do old-fashioned exponentiation
		{
			*answer = *answer  * i;
		}
    }
}




/*
 *  eval_exp7      process unary + or -
 *
 */

static void eval_exp7(int32_t *answer)
{
	register char			op; 

	op = 0;						// assume a bad token
	if ((token_type == RDP_DELIMITER)  &&
		(*token == '+' || *token == '-' || *token == '~'))	// unary op of some kind...
	{
		op = *token; 			// save the token
		get_token();			// get value to operate on
	}
	eval_exp8(answer);			// evaluate the argument
	if (op == '-')				// if doing negation...
	{
		*answer = -(*answer);
	}
	if (op == '~')				// if doing 1's complement...
	{
		*answer = ~(*answer);
	}
}



/*
 *  eval_exp8()     process parenthesized expression
 *
 */

static void  eval_exp8(int32_t *answer)
{
	if (*token == '(')  			// if doing () expression...
	{
		get_token();				// get first token in exp
		eval_exp1(answer); 			// restart parser to evaluate
		if (*token != ')')			// if exp didn't end with )
		{
			serror(RDP_UNBAL_PARENS);	// show the error
		}
		get_token();				// finish exp
	}
	else  							// not () exp, must be atom
	{
		atom(answer);				// parse the atom
	}
}




/*
 *  atom      find value of number or variable
 *
 *  This routine handles the lowest-level tokens, such as numbers.
 *  Much of this code has been stubbed out since it was used by
 *  the SBasic compiler to support features not used in a simple
 *  integer parser.  However, I've left the original SBasic
 *  additions in place so you can see how SBasic handled features
 *  like variables and funcions.
 *
 */

static void atom(int32_t *answer)
{
	int32_t		t;
//	int		fn;
//	int		usroffset, usropcode;
//	int		n;
//	int		temp;
//	int		atom_answer;
//	char	fstr[80];
	
	
	switch (token_type)  				/* based on token type... */
	{
#if 0
		case  VARIABLE : 				/* for a variable... */
		t = lookup_var(token);			/* get index of variable */
		if (vartable[t].len > 2)  {		/* if this is an array... */
			get_token();				/* this had better be a paren */
			if (*token != '(')  {		/* need a paren */
				serror(SB_ARRAY);		/* whine */
				return;
			}
			atom_answer = 0;			/* load up the parser */
			answer_flag = 1;			/* this should be a stack var! */
			eval_exp1(&atom_answer);	/* resolve index */
			get_token();				/* this had better be a paren */
			if (*token != ')')  {		/* need a paren */
				serror(SYNTAX);			/* whine */
				return;
			}
/*			t = vartable[t].offset;  */		/* now get offset */
			if (answer_flag)  {
/*				push_op(OP_LIT);  */
/*				push_op(atom_answer);  */
				push_op(OP_LIT);
				push_op(t);
				push_op(OP_GETARR);
			}
			else  {
				push_op(OP_ARRAY);			/* push the opcode */			
				push_op(t);					/* push the variable offset */
				push_op(OP_GETARR);			/* push the fetch opcode */
			}
		}
		else  {
			push_op(OP_VAR);			/* use simple variable */
			push_op(t);					/* push the variable offset */
		}
		get_token(); 					/* get the next token */
		answer_flag = 0;				/* show no valid answer */
		return; 						/* and outta here */
#endif

		case  RDP_NUMBER :				// for a number...
		t = getdec(token); 				// convert string to an int
		*answer = t;					// save for parser
		get_token();   					// get the next token
		return;							// and outta here

#if 0
		case  CONSTANT :				/* for a named constant... */
		t = lookup_const(token);		/* this always works! */
		t = consttable[t].value;		/* get the value */
		*answer = t;					/* save for parser */
		get_token();					/* get the next token */
		return;							/* and outta here */
#endif

		case  RDP_HEXNUMBER :			// for a hexadecimal number...
		t = gethex(token);				// convert hex string to int
		*answer = t;					// save for parser
		get_token();					// get the next token
		return;							// and outta here
	
		case  RDP_BINNUMBER :			// for a binary number...
		t = getbin(token);				// convert string to an int
		*answer = t;					// save for parser
		get_token();					// get the next token
		return;							// and outta here

		case  RDP_ASCNUMBER :			// for an ASCII constant...
		t = (unsigned char)(*(token+1));
		t &= 0xff;
		*answer = t;					// save for parser
		get_token();					// get the next token
		return;							// and outta here

#if 0
		case  FUNCTION :
		fn = function_num;				/* save the function ID */
		strcpy(fstr, token);			/* save function name */
		get_token();					/* get the next token */
		if (*token != '(')				/* if not a (... */
		{
			serror(NO_FUNC_ARG);		/* report an error */
			return;						/* and leave */
		}
		switch  (fn)					/* see if special action needed */
		{
			case  OP_FUNC_ADDR :		/* addr function */
			get_token();				/* get label */
			if (delim_char != ')')		/* must end with ), or error */
			{
				serror(BAD_ADDR);		/* show the error */
				return;					/* leave early */
			}
			t = lookup_var(token);		/* is it a variable? */
			if (t == -1)				/* if not... */
			{
				t = lookup_label(token);	/* is it a label? */
				if (t == -1)				/* if not found... */
				{
					t = add_label(token);	/* make it a new label */
					putback();			/* return trailing paren */
				}
				label_table[t].referenced = 1;
				push_op(OP_IMMLBL);			/* push opcode */
				push_op(t);					/* push offset */
			}
			else						/* yes, it's a variable */
			{
				push_op(OP_IMMVAR);		/* push opcode */
				push_op(t);				/* push offset */
			}
			get_token();				/* get closing paren */
			break;
		
			case  OP_FUNC_INKEY :
			case  OP_FUNC_PULL :
			case  OP_FUNC_POP :
			get_token();
			if (*token != ')')			/* if didn't get it... */
			{
				serror(EXP_PAREN);		/* complain */
				return;
			}
			push_op(OP_FUNC);
			push_op(fn);
			break;


			case  OP_FUNC_MIN :
			case  OP_FUNC_MAX :
			case  OP_FUNC_MINU :
			case  OP_FUNC_MAXU :
			eval_exp1(&temp);				/* evaluate argument 1 */
			get_token();					// should be comma
			if (*token != ',')			// if not...
			{
				serror(EXP_NUMBER);		// complain
				return;
			}
			eval_exp1(&temp);				// evaluate argument 2
			get_token();					// should be ending paren
			if (*token != ')')			// if not...
			{
				serror(EXP_PAREN);		// whine
				return;
			}
			push_op(OP_FUNC);
			push_op(fn);
			break;



			case  OP_FUNC_USR :
			get_token();
			n = lookup_var(token);		/* try to find arg in variable list */
			if (n != -1)				/* if found it... */
			{
				usropcode = OP_FUNC_USRV;	/* save the opcode */
				usroffset = n*2;		/* save the variable offset */
			}
			else
			{
				n = lookup_label(token);		/* not variable, try for label */
				if (n == -1)					/* if no such label... */
				{
					n = add_label(token);		/* add it */
					--prog;						/* repair incr in get_token() */
				}
				label_table[n].referenced = 1;		/* show label is referenced */
				usropcode = OP_FUNC_USRL;			/* save the opcode */
				usroffset = n;						/* save the label offset */
			}
			get_token();
			while (strcmp(token, ",") == 0)
			{
				exec_push();
				get_token();
			}
			if (*token != ')')				// if didn't find ending )...
			{
				serror(EXP_PAREN);			/* complain */
			}
			push_op(OP_FUNC);
			push_op(usropcode);
			push_op(usroffset);
			break;


			case  OP_FUNC_ASMFUNC :
			eval_exp1(&temp);			/* evaluate argument */
			get_token();
			if (*token != ')')			/* check for final paren */
			{
				serror(EXP_PAREN);
			}
			push_op(OP_FUNC);			/* push the function opcode */
			push_op(fn);				/* push the function ID */
			fn = lookup_func(fstr);		/* look up the function again */
			push_op(fn);				/* push pointer to function name */
			break;
			
			
			default:
			eval_exp1(&temp);			/* evaluate argument */
			get_token();
			if (*token != ')')			/* check for final paren */
			{
				serror(EXP_PAREN);
			}
			push_op(OP_FUNC);			/* push the function opcode */
			push_op(fn);				/* push the function ID */
		}
		answer_flag = 0;				/* show no valid answer */
		break;
#endif

#if 0
		case  STRING :					// this means unknown variable
		serror(NOT_VAR);				// time to whine
		break;
#endif
		default:						/* shouldn't get here */
		serror(RDP_SYNTAX); 			/* bad atom, complain */
	}
	get_token();						/* set up for next parser step */
}





/*
 *  serror      common handler for error conditions
 *
 *  In the original SBasic parser, this routine displayed a message to the
 *  console about an error condition.  In this version, the parser does not
 *  assume there is a console in use, so this routine just saves an error
 *  value to a common error variable.
 */
 
static void serror(int32_t errenum)
{
	error = errenum;
}



/*
 *  get_token      get the next token from the global string variable
 *
 *  This routine skips whitespace, then collects the following chars
 *  from the string pointed to by instr; those chars are saved to
 *  the global variable token as a null-terminated string.
 *
 *  This routine also classifies the token it finds, to help other
 *  functions in processing the token.
 *
 *  Again, I've left some of the original SBasic code in place so you
 *  can see how SBasic handled features not found in this integer-only
 *  parser.
 *
 *  Note that the global string variable being tested was named prog
 *  in the original SBasic code; in this new parser, that string is
 *  named instr.
 */
 
static uint32_t  get_token(void)
{
//	register char		*temp;
	char				*pt;
  

	token_type = RDP_UNKNOWN; 
	pt = token;							// start off at beginning of token buffer

	while ((*instr == ' ') || (*instr == '\t')) ++instr;  // skip over white space

/*
 *  An empty string is treated as a delimiter.
 */
	if (*instr == 0)					// if hit the EOL...
	{
		token[0] = '\r';
		token[1] = 0;
		token_type = RDP_DELIMITER;
	}

/*
 *  If the current char is one of the delimiters, collect the full delimiter
 *  (might be two chars!) and mark the token.
 */
	else if (rdp_strchr(DELIMITERS, *instr))	// if next char is start of delimiter...
	{
		if ((*instr == '*') && (*(instr+1) == '*'))	// special case, ** means integer exponentiation
		{
			*pt++ = *instr++;					// save first char of 2-char delimiter
		}
		*pt++ = *instr++;						// now save last (only) char of delimiter
       	*pt = 0;
		token_type = RDP_DELIMITER;
	}
    
#if 0
	if (*instr=='"')						// quoted string
	{
		prog++;
		while (*prog!='"'&& *prog!='\r') *temp++ = *prog++;
		if (*prog=='\r')  serror(MISS_QUOTE);
		prog++;
		*temp = 0;
		return (token_type=QUOTE);
	}
#endif

/*
 *  A token that starts with "0x" or "0X" is treated as a hex number.
 *  Be sure to do this test before checking for numbers, otherwise all
 *  your numbers will be 0.
 */
	else if ((*instr == '0') && (toupper((int)(*(instr+1))) == 'X'))
	{
		instr = instr + 2;					// step past 0x to first hex digit
		while (isxdigit((int)*instr))  *pt++ = *instr++;
		if (strlen(token) == 0)
		{
			serror(RDP_SYNTAX);
			token_type = RDP_UNKNOWN;
		}
		else
		{
			*pt = 0;
			token_type = RDP_HEXNUMBER;
		}
	}
  

/*
 *  A token that starts with "0b" or "0B" is treated as a binary number.
 *  Be sure to do this test before checking for numbers, otherwise all
 *  your numbers will be 0.
 */
	else if ((*instr == '0') && (toupper((int)(*(instr+1))) == 'B'))
	{
		instr = instr + 2;					// step past 0b to first binary digit
		while ((*instr == '0') || (*instr == '1'))  *pt++ = *instr++;
		if (strlen(token) == 0)
		{
			serror(RDP_SYNTAX);
			token_type = RDP_UNKNOWN;
		}
		else
		{
			*pt = 0;
			token_type = RDP_BINNUMBER;
		}
  	}


/*
 *  A token consisting of a single character enclosed in single-quotes
 *  is considered an ASCII constant.
 */
	else if ((*instr == '\'') && (*(instr+2) == '\''))	// if this is a single ASCII char...
	{
		*pt++ = *instr++;				// copy char const to token
		*pt++ = *instr++;
		*pt++ = *instr++;
		*pt = '\0';						// make it a string
		token_type = RDP_ASCNUMBER;		// return as constant
	}		

/*
 *  A token consisting of one or more ASCII digit chars is considered a
 *  decimal constant.  This code moves any preceding minus sign into the
 *  token for unary negation.
 */
	else if (isdigit((int)*instr) || (*instr == '-'))				/* number */
	{
		*pt++ = *instr++;
		while (isdigit((int)*instr))  *pt++ = *instr++;
		*pt = '\0';
		token_type = RDP_NUMBER;
	}

#if 0
	if (*prog == '\'')  {					/* remark char */
		*temp++ = *prog++;
		*temp = '\0';
		tok = REMCHAR;
		return (token_type = COMMAND);
	}
#endif

	
#if 0
	if (isalpha(*prog) || (*prog == '_'))  {	/* var or command */
		while  (!isdelim(*prog))				/* copy string to token */
			*temp++ = *prog++;
	}
	delim_char = *prog;							/* preserve delimiter */

	*temp = '\0';								/* end the string */
	token_type = STRING;						/* set token type */


/*
 *  see if a string is a command or a variable
 */
 
	if (token_type==STRING)  {
		tok = look_up(token);				/* convert to internal rep */
		if (tok == TO)  {					/* special case! */
			if (*prog == '*')  {			/* look for unsigned TO */
				*temp++ = *prog++;			/* move the * */
				*temp = '\0';				/* end the string */
				tok = TOSTAR;				/* show new token */
			}
		}

		if (tok)  token_type = COMMAND;
	    else  {
    		n = lookup_const(token);
			if (n != -1)  {
/*    			token_type = NUMBER;  */
    			token_type = CONSTANT;
/*				sprintf(token, "%d", consttable[n].value);	*/ /* new */
/*				answer = consttable[n].value;  */
    		}
    		else  {
    			n = lookup_func(token);
				if (n != -1)  {
    				token_type = FUNCTION;
    				function_num = functable[n].value;
    			}
	    		else  {
	    			if (*(token+strlen(token)-1) == ':')  {		/* if label */
						*(token+strlen(token)-1) = '\0';	/* remove : */
		    			n = lookup_label(token);	/* try to find label */
						if (n != -1)  {				/* if in table... */
		    				if (label_table[n].defined)  {	/* and defined...*/
		    					serror(DUP_LAB);		/* that's an error */
		    				}
		    				else  {
		    					label_table[n].defined = 1;
		    				}
		    			}
		    			else  {
		    				n = add_label(token);		/* add label */
			    			label_table[n].defined = 1;		/* show it's defined */
			    			if (strcmp(token, "main") == 0)  {
			    				label_table[n].referenced = 1;
			    			}
			    		}
		    			tok = n;					/* pass back label index */
		    			token_type = LABEL;
					}
					else  {
		    			n = lookup_label(token);	/* try to find label */
						if (n != -1)  {				/* if in table... */
							token_type = LABEL;		/* show a label */
							label_table[n].referenced = 1;
						}
						else  {
							n = lookup_var(token);
							if (n != -1)  {
					    		token_type = VARIABLE;	/* show a variable */
							}
				    	}
			    	}
				}
			}
		}
	}
	if (token_type == STRING)  {		/* if still not resolved... */
		prog++;							/* prevent lockup */
	}
#endif

	return  token_type;
}



/*
 *  putback      return a token to input stream
 */
 
static void putback(void) 
{

	char			*t; 

	t = token; 
	for (; *t; t++)
	{
		instr--;
	}
}




static int32_t  getbin(char  *str)
{
	int 	t;

	t = 0;
	while ((*str == '0') || (*str == '1'))
	{
		t = (t << 1) + (*str++ - '0');
	}
	return  t;
}


static int32_t  gethex(char  *str)
{
	int32_t			t;
	int32_t			c;

	t = 0;
	while (isxdigit((int)*str))
	{
		c = (toupper((int)*str) - '0');
		if (c > 10)  c = c - 7;
		t = (t * 16) + c;
		str++;
	}
	return  t;
}


static int32_t  getdec(char  *str)
{
	int32_t			t;
	int32_t			c;
	uint8_t			negflag;

	t = 0;
	negflag = 0;
	if (*str == '-')
	{
		negflag = 1;
		str++;
	}
	while (isdigit((int)*str))
	{
		c = *str - '0';
		t = (t * 10) + c;
		str++;
	}
	if (negflag)  t = -t;
	return  t;
}


/*
 *  rdp_strchr      test for presence of a char within a string
 *
 *  This is a version of strchr() that does not require any newlib
 *  functions.  If the rest of your platform already supports newlib
 *  syscalls, you can replace this function with the stock strchr().
 */
static char  *rdp_strchr(char  *str, char  c)
{
	while (*str)
	{
		if (*str == c)  return  str;
		str++;
	}
	return  0;
}




