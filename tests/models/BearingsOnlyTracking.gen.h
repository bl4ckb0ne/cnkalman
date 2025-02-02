static inline FLT gen_bearings_meas_function(const FLT* state, const FLT* landmark) {
	const FLT state0 = state[0];
	const FLT state1 = state[1];
	const FLT landmark0 = landmark[0];
	const FLT landmark1 = landmark[1];

	return atan2(state1 + (-1 * landmark1), state0 + (-1 * landmark0));
}

// Jacobian of bearings_meas_function wrt [state0, state1]
static inline void gen_bearings_meas_function_jac_state(CnMat* Hx, const FLT* state, const FLT* landmark) {
	const FLT state0 = state[0];
	const FLT state1 = state[1];
	const FLT landmark0 = landmark[0];
	const FLT landmark1 = landmark[1];
	const FLT x0 = state1 + (-1 * landmark1);
	const FLT x1 = state0 + (-1 * landmark0);
	const FLT x2 = 1. / ((x1 * x1) + (x0 * x0));
	cnMatrixSet(Hx, 0, 0, -1 * x0 * x2);
	cnMatrixSet(Hx, 0, 1, x2 * x1);
}

// Full version Jacobian of bearings_meas_function wrt [state0, state1]
static inline void gen_bearings_meas_function_jac_state_with_hx(CnMat* Hx, CnMat* hx, const FLT* state, const FLT* landmark) {
    if(Hx == 0) { 
        hx->data[0] = gen_bearings_meas_function(state, landmark);
        return;
    }
    if(hx == 0) { 
        gen_bearings_meas_function_jac_state(Hx, state, landmark);
        return;
    }
	const FLT state0 = state[0];
	const FLT state1 = state[1];
	const FLT landmark0 = landmark[0];
	const FLT landmark1 = landmark[1];
	const FLT x0 = state1 + (-1 * landmark1);
	const FLT x1 = state0 + (-1 * landmark0);
	const FLT x2 = 1. / ((x1 * x1) + (x0 * x0));
	cnMatrixSet(Hx, 0, 0, -1 * x0 * x2);
	cnMatrixSet(Hx, 0, 1, x2 * x1);
	cnMatrixSet(hx, 0, 0, atan2(x0, x1));
}

// Jacobian of bearings_meas_function wrt [landmark0, landmark1]
static inline void gen_bearings_meas_function_jac_landmark(CnMat* Hx, const FLT* state, const FLT* landmark) {
	const FLT state0 = state[0];
	const FLT state1 = state[1];
	const FLT landmark0 = landmark[0];
	const FLT landmark1 = landmark[1];
	const FLT x0 = state1 + (-1 * landmark1);
	const FLT x1 = state0 + (-1 * landmark0);
	const FLT x2 = 1. / ((x1 * x1) + (x0 * x0));
	cnMatrixSet(Hx, 0, 0, x0 * x2);
	cnMatrixSet(Hx, 0, 1, -1 * x2 * x1);
}

// Full version Jacobian of bearings_meas_function wrt [landmark0, landmark1]
static inline void gen_bearings_meas_function_jac_landmark_with_hx(CnMat* Hx, CnMat* hx, const FLT* state, const FLT* landmark) {
    if(Hx == 0) { 
        hx->data[0] = gen_bearings_meas_function(state, landmark);
        return;
    }
    if(hx == 0) { 
        gen_bearings_meas_function_jac_landmark(Hx, state, landmark);
        return;
    }
	const FLT state0 = state[0];
	const FLT state1 = state[1];
	const FLT landmark0 = landmark[0];
	const FLT landmark1 = landmark[1];
	const FLT x0 = state1 + (-1 * landmark1);
	const FLT x1 = state0 + (-1 * landmark0);
	const FLT x2 = 1. / ((x1 * x1) + (x0 * x0));
	cnMatrixSet(Hx, 0, 0, x0 * x2);
	cnMatrixSet(Hx, 0, 1, -1 * x2 * x1);
	cnMatrixSet(hx, 0, 0, atan2(x0, x1));
}

