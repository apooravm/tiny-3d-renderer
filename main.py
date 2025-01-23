principal_amt = input("Principal Amount: ")
interest_rate = input("Interest Rate: ")
num_applied = input("Number of times interest rate applied: ")
num_elapsed = input("Number of time periods elapsed: ")

result = pow((1 + (interest_rate / num_applied)), num_applied * num_elapsed) * principal_amt

jobs = [
    {
        "title": "",
        "company": "",
        "description": "",
        "salary": 0,
        "status": False,
    }
]

