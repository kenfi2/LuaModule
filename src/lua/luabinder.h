namespace luabinder
{
	
	template<typename T> struct remove_const_ref { typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type; };

	template<typename Lambda>
	struct lambda_to_stdfunction {
		template<typename F>
		struct convert_lambda;

		template<typename L, typename Ret, typename... Args>
		struct convert_lambda<Ret(L::*)(Args...) const> {
			typedef std::function<Ret(Args...)> value;
		};

		typedef decltype(&Lambda::operator()) F;
		typedef typename convert_lambda<F>::value value;
	};

	template<int N>
	struct pack_values_into_tuple {
		template<typename Tuple>
		static void call(Tuple& tuple, LuaInterface* lua) {
			typedef typename std::tuple_element<N - 1, Tuple>::type ValueType;
			std::get<N - 1>(tuple) = polymorphic_pop<ValueType>(lua->getLuaState());
			pack_values_into_tuple<N - 1>::call(tuple, lua);
		}
	};
	template<>
	struct pack_values_into_tuple<0> {
		template<typename Tuple>
		static void call(Tuple& /*tuple*/, LuaInterface* /*L*/) { }
	};

	/// C++ function caller that can push results to lua
	template<typename Ret, typename F, typename... Args>
	typename std::enable_if<!std::is_void<Ret>::value, int>::type
		call_fun_and_push_result(const F& f, LuaInterface* lua, const Args&... args) {
		Ret ret = f(args...);
		int numRets = polymorphic_push(lua->getLuaState(), ret);
		return numRets;
	}

	/// C++ void function caller
	template<typename Ret, typename F, typename... Args>
	typename std::enable_if<std::is_void<Ret>::value, int>::type
		call_fun_and_push_result(const F& f, LuaInterface* lua, const Args&... args) {
		f(args...);
		return 0;
	}

	/// Expand arguments from tuple for later calling the C++ function
	template<int N, typename Ret>
	struct expand_fun_arguments {
		template<typename Tuple, typename F, typename... Args>
		static int call(const Tuple& tuple, const F& f, LuaInterface* lua, const Args&... args) {
			return expand_fun_arguments<N - 1, Ret>::call(tuple, f, lua, std::get<N - 1>(tuple), args...);
		}
	};
	template<typename Ret>
	struct expand_fun_arguments<0, Ret> {
		template<typename Tuple, typename F, typename... Args>
		static int call(const Tuple&, const F& f, LuaInterface* lua, const Args&... args) {
			return call_fun_and_push_result<Ret>(f, lua, args...);
		}
	};

	template<typename Ret, typename F, typename Tuple>
	LuaCppFunction bind_fun_specializer(const F& f) {
		enum { N = std::tuple_size<Tuple>::value };
		return [=](LuaInterface* lua) -> int {
			lua_State* L = lua->getLuaState();
			while (lua_gettop(L) != N) {
				if (lua_gettop(L) < N)
					lua_pushnil(L);
				else
					lua_pop(L, 1);
			}

			Tuple tuple;
			pack_values_into_tuple<N>::call(tuple, lua);
			return expand_fun_arguments<N, Ret>::call(tuple, f, lua);
		};
	}

	/// Create member function lambdas for singleton classes
	template<typename Ret, typename C, typename... Args>
	std::function<Ret(const Args&...)> make_mem_func_singleton(Ret(C::* f)(Args...), C* instance) {
		auto mf = std::mem_fn(f);
		return [=](Args... args) mutable -> Ret { return mf(instance, args...); };
	}
	template<typename C, typename... Args>
	std::function<void(const Args&...)> make_mem_func_singleton(void (C::* f)(Args...), C* instance) {
		auto mf = std::mem_fn(f);
		return [=](Args... args) mutable -> void { mf(instance, args...); };
	}

	/// Bind singleton member functions
	template<typename C, typename Ret, class FC, typename... Args>
	LuaCppFunction bind_singleton_mem_fun(Ret(FC::* f)(Args...), C* instance) {
		typedef typename std::tuple<typename remove_const_ref<Args>::type...> Tuple;
		assert(instance);
		auto lambda = make_mem_func_singleton<Ret, FC>(f, static_cast<FC*>(instance));
		return bind_fun_specializer<typename remove_const_ref<Ret>::type,
			decltype(lambda),
			Tuple>(lambda);
	}

	/// Bind singleton member functions
	template<typename C, class FC>
	LuaCppFunction bind_singleton_mem_fun(int (FC::* f)(LuaInterface*), C* instance) {
		assert(instance);
		auto mf = std::mem_fn(f);
		return [=](LuaInterface* lua) mutable -> int { return mf(instance, lua); };
	}

	/// Bind a std::function
	template<typename Ret, typename... Args>
	LuaCppFunction bind_fun(const std::function<Ret(Args...)>& f) {
		typedef typename std::tuple<typename remove_const_ref<Args>::type...> Tuple;
		return bind_fun_specializer<typename remove_const_ref<Ret>::type,
			decltype(f),
			Tuple>(f);
	}

	/// Specialization for lambdas
	template<typename F>
	struct bind_lambda_fun;

	template<typename Lambda, typename Ret, typename... Args>
	struct bind_lambda_fun<Ret(Lambda::*)(Args...) const> {
		static LuaCppFunction call(const Lambda& f) {
			typedef typename std::tuple<typename remove_const_ref<Args>::type...> Tuple;
			return bind_fun_specializer<typename remove_const_ref<Ret>::type,
				decltype(f),
				Tuple>(f);
		}
	};

	template<typename Lambda>
	typename std::enable_if<std::is_constructible<decltype(&Lambda::operator())>::value, LuaCppFunction>::type bind_fun(const Lambda& f) {
		typedef decltype(&Lambda::operator()) F;
		return bind_lambda_fun<F>::call(f);
	}

	template<class C, typename F>
	void bindMethodFunction(const std::string& className, const std::string& functionName, F C::* function, C* instance)
	{
		lua_State* L = g_lua.getLuaState();
		if (!L)
			return;

		lua_getglobal(L, className.c_str());
		if (lua_isnil(L, -1)) {
			std::cout << className << " a nil value." << std::endl;
			lua_pop(L, 1);
			return;
		}

		LuaInterface::pushCppFunction(L, bind_singleton_mem_fun(function, instance), className + "." + functionName);
		lua_setfield(L, -2, functionName.c_str());
		lua_pop(L, 1);
	}

	template<typename F>
	void bindFunction(const std::string& functionName, const F& function)
	{
		lua_State* L = g_lua.getLuaState();
		if (!L)
			return;

		LuaInterface::pushCppFunction(L, bind_fun(function), functionName);
		lua_setglobal(L, functionName.c_str());
	}
}
