#pragma once
#include<string>
#include<functional>
#include<stdexcept>
#include"sqlite3.h"


namespace sqlite {

	using namespace std;
	class database {
	private:
		sqlite3 * _db;
		bool _connected;
	public:
		class database_binder {
		private:
			sqlite3 * _db;
			wstring _sql;
			sqlite3_stmt* _stmt;
			int _inx;
			//template<typename T> class type_call { };

			void _extract(function<void(void)> call_back){
				int hresult;
				while ((hresult = sqlite3_step(_stmt)) == SQLITE_ROW)
					call_back();

				if (hresult != SQLITE_DONE)
					throw exception(sqlite3_errmsg(_db));

				if (sqlite3_finalize(_stmt) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				_stmt = nullptr;
			}
			void _extract_single_value(function<void(void)> call_back){
				int hresult;
				if ((hresult = sqlite3_step(_stmt)) == SQLITE_ROW)
					call_back();

				if ((hresult = sqlite3_step(_stmt)) == SQLITE_ROW)
					throw exception("not every row extracted");

				if (hresult != SQLITE_DONE)
					throw exception(sqlite3_errmsg(_db));

				if (sqlite3_finalize(_stmt) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				_stmt = nullptr;
			}
			void _prepare(){
				if (sqlite3_prepare16_v2(_db, _sql.data(), -1, &_stmt, nullptr) != SQLITE_OK)
					throw std::exception(sqlite3_errmsg(_db));
			}
		protected:
			database_binder(sqlite3 * db, wstring const & sql) :
				_db(db),
				_sql(sql),
				_stmt(nullptr),
				_inx(1)
			{
				_prepare();
			}

			database_binder(sqlite3 * db, string const & sql) :
				_db(db),
				_sql(sql.begin(), sql.end()),
				_stmt(nullptr),
				_inx(1)
			{
				_prepare();
			}

		public:
			friend class database;
			~database_binder(){
				if (_stmt){
					int hresult;
					if ((hresult = sqlite3_step(_stmt)) != SQLITE_DONE){
						if (hresult == SQLITE_ROW)
							throw exception("not all rows readed");
						throw exception(sqlite3_errmsg(_db));
					}
					if (sqlite3_finalize(_stmt) != SQLITE_OK)
						throw exception(sqlite3_errmsg(_db));
					_stmt = nullptr;
				}
			}
#pragma region operator <<
			database_binder& operator <<(double val) {
				if (sqlite3_bind_double(_stmt, _inx, val) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
			database_binder& operator <<(float val) {
				if (sqlite3_bind_double(_stmt, _inx, double(val)) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
			database_binder& operator <<(int val) {
				if (sqlite3_bind_int(_stmt, _inx, val) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
			database_binder& operator <<(sqlite_int64 val) {
				if (sqlite3_bind_int64(_stmt, _inx, val) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
			database_binder& operator <<(string const& txt) {
				if (sqlite3_bind_text(_stmt, _inx, txt.data(), -1, SQLITE_STATIC) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
			database_binder& operator <<(wstring const& txt) {
				if (sqlite3_bind_text16(_stmt, _inx, txt.data(), -1, SQLITE_STATIC) != SQLITE_OK)
					throw exception(sqlite3_errmsg(_db));
				++_inx;
				return *this;
			}
#pragma endregion


#pragma region get_col_from_db			
			void get_col_from_db(int inx, int& i){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL) i = 0;
				else i = sqlite3_column_int(_stmt, inx);
			}
			void get_col_from_db(int inx, sqlite3_int64& i){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL)
					i = 0;
				else i = sqlite3_column_int64(_stmt, inx);
			}
			void get_col_from_db(int inx, string& s){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL) s = string();
				sqlite3_column_bytes(_stmt, inx);
				s = string((char*)sqlite3_column_text(_stmt, inx));
			}
			void get_col_from_db(int inx, wstring& w){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL) w = wstring();
				sqlite3_column_bytes16(_stmt, inx);
				w = wstring((wchar_t *)sqlite3_column_text16(_stmt, inx));
			}
			void get_col_from_db(int inx, double& d){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL)
					d = 0;
				d = sqlite3_column_double(_stmt, inx);
			}
			void get_col_from_db(int inx, float& f){
				if (sqlite3_column_type(_stmt, inx) == SQLITE_NULL)
					f = 0;
				f = float(sqlite3_column_double(_stmt, inx));
			}
#pragma endregion


#pragma region operator >>

			void operator>>(int & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}
			void operator>>(string & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}
			void operator>>(wstring & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}
			void operator>>(double & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}
			void operator>>(float & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}
			void operator>>(sqlite3_int64 & val){
				_extract_single_value([&]{
					get_col_from_db(0, val);
				});
			}

			template<typename FUNC>
			void operator>>(FUNC l){
				typedef function_traits<decltype(l)> traits;

				database_binder& dbb = *this;
				_extract([&](){
					binder<traits::arity>::run(dbb, l);
				});

			}
#pragma endregion

		};

		database(wstring const & db_name) : _db(nullptr), _connected(false) {
			_connected = sqlite3_open16(db_name.data(), &_db) == SQLITE_OK;
		}
		database(string const & db_name) : _db(nullptr), _connected(false) {
			_connected = sqlite3_open16(wstring(db_name.begin(), db_name.end()).data(), &_db) == SQLITE_OK;
		}

		~database(){
			if (_db){
				sqlite3_close_v2(_db);
				_db = nullptr;
			}
		}

		database_binder operator<<(string const& sql) const{
			return database_binder(_db, sql);
		}
		database_binder operator<<(wstring const& sql) const{
			return database_binder(_db, sql);
		}

		operator bool() const{
			return _connected;
		}
		operator string() {
			return sqlite3_errmsg(_db);
		}
		operator wstring() {
			return (wchar_t*)sqlite3_errmsg16(_db);
		}



		template <typename T>
		struct function_traits
			: public function_traits<decltype(&T::operator())>
		{};

		template <typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) const>
			// we specialize for pointers to member function
		{
			enum { arity = sizeof...(Args) };
			// arity is the number of arguments.

			typedef ReturnType result_type;

			template <size_t i>
			struct arg
			{
				typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
				// the i-th argument is equivalent to the i-th tuple element of a tuple
				// composed of those arguments.
			};
		};


		template<int N>
		class binder {
			template<typename F>
			static void run(database_binder& dbb, F l);
		};
		template<>
		struct binder<0> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;

				l();
			}
		};
		template<>
		struct binder<1> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);

				l(move(col_1));
			}
		};
		template<>
		struct binder<2> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;

				type_1 col_1;
				type_2 col_2;
				dbb.get_col_from_db(0, col_1);
				dbb.get_col_from_db(1, col_2);

				l(move(col_1), move(col_2));
			}
		};
		template<>
		struct binder<3> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);

				l(move(col_1), move(col_2), move(col_3));
			}
		};
		template<>
		struct binder<4> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);

				l(move(col_1), move(col_2), move(col_3), move(col_4));
			}
		};
		template<>
		struct binder<5> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5));
			}
		};
		template<>
		struct binder<6> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;
				typedef typename traits::arg<5>::type type_6;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);
				type_6 col_6;
				dbb.get_col_from_db(5, col_6);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5), move(col_6));
			}
		};
		template<>
		struct binder<7> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;
				typedef typename traits::arg<5>::type type_6;
				typedef typename traits::arg<6>::type type_7;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);
				type_6 col_6;
				dbb.get_col_from_db(5, col_6);
				type_7 col_7;
				dbb.get_col_from_db(6, col_7);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5), move(col_6), move(col_7));
			}
		};
		template<>
		struct binder<8> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;
				typedef typename traits::arg<5>::type type_6;
				typedef typename traits::arg<6>::type type_7;
				typedef typename traits::arg<7>::type type_8;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);
				type_6 col_6;
				dbb.get_col_from_db(5, col_6);
				type_7 col_7;
				dbb.get_col_from_db(6, col_7);
				type_8 col_8;
				dbb.get_col_from_db(7, col_8);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5), move(col_6), move(col_7), move(col_8));
			}
		};
		template<>
		struct binder<9> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;
				typedef typename traits::arg<5>::type type_6;
				typedef typename traits::arg<6>::type type_7;
				typedef typename traits::arg<7>::type type_8;
				typedef typename traits::arg<8>::type type_9;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);
				type_6 col_6;
				dbb.get_col_from_db(5, col_6);
				type_7 col_7;
				dbb.get_col_from_db(6, col_7);
				type_8 col_8;
				dbb.get_col_from_db(7, col_8);
				type_9 col_9;
				dbb.get_col_from_db(8, col_9);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5), move(col_6), move(col_7), move(col_8), move(col_9));
			}
		};
		template<>
		struct binder<10> {
			template<typename F>
			static void run(database_binder& dbb, F l) {
				typedef function_traits<decltype(l)> traits;
				typedef typename traits::arg<0>::type type_1;
				typedef typename traits::arg<1>::type type_2;
				typedef typename traits::arg<2>::type type_3;
				typedef typename traits::arg<3>::type type_4;
				typedef typename traits::arg<4>::type type_5;
				typedef typename traits::arg<5>::type type_6;
				typedef typename traits::arg<6>::type type_7;
				typedef typename traits::arg<7>::type type_8;
				typedef typename traits::arg<8>::type type_9;
				typedef typename traits::arg<9>::type type_10;

				type_1 col_1;
				dbb.get_col_from_db(0, col_1);
				type_2 col_2;
				dbb.get_col_from_db(1, col_2);
				type_3 col_3;
				dbb.get_col_from_db(2, col_3);
				type_4 col_4;
				dbb.get_col_from_db(3, col_4);
				type_5 col_5;
				dbb.get_col_from_db(4, col_5);
				type_6 col_6;
				dbb.get_col_from_db(5, col_6);
				type_7 col_7;
				dbb.get_col_from_db(6, col_7);
				type_8 col_8;
				dbb.get_col_from_db(7, col_8);
				type_9 col_9;
				dbb.get_col_from_db(8, col_9);
				type_9 col_10;
				dbb.get_col_from_db(9, col_10);

				l(move(col_1), move(col_2), move(col_3), move(col_4), move(col_5), move(col_6), move(col_7), move(col_8), move(col_9), move(col_10));
			}
		};
	};

}
