// ReNFA.cpp : Defines the entry point for the console application.
// https://swtch.com/~rsc/regexp/regexp1.html

#include <stack>
#include <queue>
#include <map>
#include <list>
#include <string>
#include <iostream>
using namespace std;

#pragma region RegEx to Postfix
// (aa)(bb|(cc|dd))+ee?
// use tow stack to handle this problem

bool combine(stack<string>& stackOp, char nextOp)
{
	if (nextOp == '?' || nextOp == '*' || nextOp == '+')
	{
		return false;
	}

	if (stackOp.size() >= 2)
	{
		string op2 = stackOp.top(); stackOp.pop();
		string op1 = stackOp.top(); stackOp.pop();

		if (op1 != string("(") && op1 != string("|"))
		{
			stackOp.push(op1 + op2 + ".");
		}
		else
		{
			stackOp.push(op1);
			stackOp.push(op2);
		}
	}

	return true;
}

string re2postfix(const char* re)
{
	string strRet = "";

	if (re == NULL)
	{
		return strRet;
	}

	stack<string> stackOperator;
	stack<string> stackOperand;
	const char* pC = re;
	while (*pC != '\0')
	{
		switch (*pC)
		{
		default:
			{
				stackOperator.push(string(1, *pC));
				combine(stackOperator, *(pC+1));
			}
			break;
		case '(':
			{
				stackOperator.push(string(1, *pC));
			}
			break;
		case ')':
			{
				string op = "";
				do
				{
					if (stackOperator.empty())
					{
						cout << "Wrong regex. 1111" << endl;
						return strRet;;
					}

					if (op != "")
					{
						stackOperand.push(op);
					}

					op = stackOperator.top(); stackOperator.pop();
				} 
				while (op != "(");

				bool bNeedCombine = false;
				while (!stackOperand.empty())
				{
					op = stackOperand.top(); stackOperand.pop();
					if (op == "|")
					{
						if (stackOperator.empty() || stackOperand.empty())
						{
							cout << "Wrong regex. 2222" << endl;
							return strRet;;
						}
						string opL = stackOperator.top(); stackOperator.pop();
						string opR = stackOperand.top(); stackOperand.pop();
						stackOperator.push(opL + opR + "|");
					}
					else
					{
						stackOperator.push(op);
					}

					bNeedCombine = true;
				}
				if (bNeedCombine)
				{
					combine(stackOperator, *(pC+1));
				}
			}
			break;
		case '|':
			{
				stackOperator.push(string(1, *pC));
			}
			break;
		case '?':
		case '*':
		case '+':
			{
				if (stackOperator.empty())
				{
					cout << "Wrong regex. 3333" << endl;
					return strRet;;
				}

				string op = stackOperator.top(); stackOperator.pop();
#if 0
				char lastOp = op[op.length() - 1];
				if (lastOp == '?' || lastOp == '*' || lastOp == '+')
				{
					cout << "Wrong regex. 4444" << endl;
					return strRet;;
				}
#endif

				stackOperator.push(op + string(1, *pC));
				combine(stackOperator, *(pC+1));
			}
			break;
		}
		pC++;
	}
	if (stackOperator.size() != 1)
	{
		cout << "Wrong regex. 5555" << endl;
		return strRet;
	}

	strRet = stackOperator.top(); stackOperator.pop();
	return strRet;
}

#pragma endregion


#pragma region Postfix to NFA

struct State
{
	int m_iFunction;
	State* m_pOut1;
	State* m_pOut2;

	State(int iFunction, State* pOut1, State* pOut2)
	{
		m_iFunction = iFunction;
		m_pOut1 = pOut1;
		m_pOut2 = pOut2;
	}
};

struct Frag
{
	State* m_pStart;
	State** m_pOut;

	Frag(State* pStart, State** pOut)
	{
		m_pStart = pStart;
		m_pOut = pOut;
	}
};

enum Function
{
	Match = 256,
	Split = 257,
};

State StateMatch = State(Match, NULL, NULL);

// 后缀表达式到NFA
State* post2nfa(const char* postfix)
{
	State* pRet = NULL;

	stack<Frag> stackFrag;

	const char* pC = postfix;
	for (; pC != NULL && *pC != '\0'; pC++)
	{
		switch (*pC)
		{
		case '.':
			{
				Frag stF2 = stackFrag.top(); stackFrag.pop();
				Frag stF1 = stackFrag.top(); stackFrag.pop();

				State** ppState = stF1.m_pOut;
				while (ppState != NULL)
				{
					State* pTmp = *ppState;

					*ppState = stF2.m_pStart;

					ppState = (State**)pTmp;
				}

				stackFrag.push(Frag(stF1.m_pStart, stF2.m_pOut));
			}
			break;
		case '|':
			{
				Frag stF2 = stackFrag.top(); stackFrag.pop();
				Frag stF1 = stackFrag.top(); stackFrag.pop();

				State* pState = new State(Split, NULL, NULL);
				pState->m_pOut1 = stF1.m_pStart;
				pState->m_pOut2 = stF2.m_pStart;

				State** ppState = stF1.m_pOut;
				while (*ppState != NULL)
				{
					ppState = (State**)(*ppState);
				}
				*ppState = (State*)stF2.m_pOut;

				stackFrag.push(Frag(pState, stF1.m_pOut));
			}
			break;
		case '?':
			{
				Frag stF = stackFrag.top(); stackFrag.pop();
				
				State* pState = new State(Split, NULL, NULL);
				pState->m_pOut1 = stF.m_pStart;
				pState->m_pOut2 = NULL;

				State** ppState = stF.m_pOut;
				while (*ppState != NULL)
				{
					ppState = (State**)(*ppState);
				}
				*ppState = (State*)(&pState->m_pOut2);

				stackFrag.push(Frag(pState, stF.m_pOut));
			}
			break;
		case '*':
			{
				Frag stF = stackFrag.top(); stackFrag.pop();

				State* pState = new State(Split, NULL, NULL);
				pState->m_pOut1 = stF.m_pStart;
				pState->m_pOut2 = NULL;

				State** ppState = stF.m_pOut;
				while (ppState != NULL)
				{
					State* pTmp = *ppState;

					*ppState = pState;

					ppState = (State**)pTmp;
				}

				stackFrag.push(Frag(pState, &pState->m_pOut2));
			}
			break;
		case '+':
			{
				Frag stF = stackFrag.top(); stackFrag.pop();

				State* pState = new State(Split, NULL, NULL);
				pState->m_pOut1 = stF.m_pStart;
				pState->m_pOut2 = NULL;

				State** ppState = stF.m_pOut;
				while (ppState != NULL)
				{
					State* pTmp = *ppState;

					*ppState = pState;

					ppState = (State**)pTmp;
				}

				stackFrag.push(Frag(stF.m_pStart, &pState->m_pOut2));
			}
			break;
		default:
			{
				State* pState = new State(*pC, NULL, NULL);
				stackFrag.push(Frag(pState, &pState->m_pOut1));
			}
			break;
		}
	}

	if (!stackFrag.empty())
	{
		Frag stF = stackFrag.top(); stackFrag.pop();

		pRet = stF.m_pStart;

		State** ppState = stF.m_pOut;
		while (ppState != NULL)
		{
			State* pTmp = *ppState;

			*ppState = &StateMatch;

			ppState = (State**)pTmp;
		}
	}
	
	return pRet;
}

#pragma endregion

#pragma region Match by NFA
void pushBack(list<State*>& listCheckState, State* pState)
{
	if (pState != NULL)
	{
		if (pState->m_iFunction == Split)
		{
			pushBack(listCheckState, pState->m_pOut1);
			pushBack(listCheckState, pState->m_pOut2);
		}
		else
		{
			listCheckState.push_back(pState);
		}
	}
}

void matchByNFA(State* pNFAStart, const char* pStrFoo)
{
	if (pStrFoo == NULL)
	{
		return;
	}

	list<State*> listCheckStateCurr;
	list<State*> listCheckStateNext;

	pushBack(listCheckStateNext, pNFAStart);

	const char* pStr = pStrFoo;
	string str4print = "";
	while (!listCheckStateNext.empty())
	{
		listCheckStateCurr.assign(listCheckStateNext.begin(), listCheckStateNext.end());

		listCheckStateNext.clear();

		list<State*>::iterator iter = listCheckStateCurr.begin();
		for (; iter != listCheckStateCurr.end(); iter++)
		{
			State* pState = *iter;
			switch (pState->m_iFunction)
			{
			case Match:
				cout << str4print << endl;
				break;
			case Split:
				cout << "Something must be error." << endl;
				break;
			default:
				if (pState->m_iFunction == *pStr)
				{
					// 满足，添加到下一步CheckList
					pushBack(listCheckStateNext, pState->m_pOut1);
					pushBack(listCheckStateNext, pState->m_pOut2);
				}
				break;
			}
		}

		str4print += string(1, *pStr);
		pStr++;
	}
}
#pragma endregion

int main(int argc, char* argv[])
{
	cout << "========== welcome ==========" << endl;
	cout << "input RegEx and the Target String:" << endl;

	string strRe = "";
	string strMatchFoo = "";
	while (cin >> strRe >> strMatchFoo)
	{
		cout << "RegEx origin  : " << strRe << endl;
		string postfix = re2postfix(strRe.c_str());
		cout << "RegEx postfix : " << postfix << endl;
	    
		cout << "matchFoo : " << strMatchFoo << endl;

		State* pNFA = post2nfa(postfix.c_str());
		cout << "--------- match list ---------" << endl;
		const char* pMatchFoo = strMatchFoo.c_str();
		while (*pMatchFoo != '\0')
		{
			cout << ">>>>> next substring --- " << pMatchFoo << endl;

			matchByNFA(pNFA, pMatchFoo);

			pMatchFoo++;
		}
		cout << "--------- match list ---------" << endl;
	}

	return 0;
}

