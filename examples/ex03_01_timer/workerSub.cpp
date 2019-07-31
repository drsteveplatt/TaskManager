// workerSub -- a subroutine to call 
// By calling an external subroutine, we can prevent optimizers from optimizing loops
// out of existence

void workerSub(byte* param) {
  param++;
}
