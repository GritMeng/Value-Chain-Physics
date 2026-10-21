import React, { useState, useRef, useEffect } from 'react';
import { Bot, Send, User } from 'lucide-react';
import { useScenario } from '../contexts/ScenarioContext';

const AiCopilot = () => {
  const { selectedPartCode, selectedDemandId, activeTab, currentScenario } = useScenario();
  const [messages, setMessages] = useState([
    {
      role: 'assistant',
      content:
        '您好！我是您的 <b>五维心智物理镜像 AI 协同大脑</b>。我已将底层的 <b>C++ 裸金属消纳规划引擎 (L0 OS)</b> 与 <b>DuckDB 物理镜像活表数据库</b> 直连。针对当前多级价值链的交付阻抗、变分自由能异常或决策规则参数，您需要我进行哪些主动推断？（例如您可提问：“为什么 Normal-FG 延误了？”、“分析大盘共识营收对账”等）',
    },
  ]);
  const [input, setInput] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const messagesEndRef = useRef(null);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSend = async (e) => {
    e.preventDefault();
    if (!input.trim() || isLoading) return;

    const userMessage = input.trim();
    setInput('');
    setMessages((prev) => [...prev, { role: 'user', content: userMessage }]);
    setIsLoading(true);

    try {
      const res = await fetch('/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          query: userMessage,
          selectedPartCode,
          selectedDemandId,
          activeTab,
          currentScenario
        }),
      });
      const data = await res.json();
      
      // Parse backend response which contains markdown
      // We will render it directly as text with line breaks and markdown-like parsing
      setMessages((prev) => [
        ...prev,
        { role: 'assistant', content: data.response || data.message || '收到，已重算并更新。' },
      ]);
    } catch (err) {
      console.error('Error in chat:', err);
      setMessages((prev) => [
        ...prev,
        { role: 'assistant', content: '❌ 请求失败，请检查网络或后端服务连接。' },
      ]);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="workbench-card p-3 flex flex-col h-full overflow-hidden">
      <div className="border-b border-main pb-2 mb-2 flex items-center justify-between">
        <h4 className="font-bold text-heading text-xs flex items-center">
          <Bot className="w-4 h-4 text-indigo-500 mr-1.5 animate-pulse" />
          五维心智物理镜像 AI 协同大脑 (Cognitive Copilot)
        </h4>
        <span className="text-[9px] text-muted cyber-font">SYS_COPILOT: ACTIVE</span>
      </div>

      {/* Messages list */}
      <div className="flex-1 overflow-y-auto space-y-3 pr-1 text-xs">
        {messages.map((msg, idx) => (
          <div
            key={idx}
            className={`flex items-start space-x-2 p-2 rounded ${
              msg.role === 'user'
                ? 'bg-indigo-950/20 border border-indigo-900/40 ml-4'
                : 'bg-slate-900/40 border border-slate-800/40 mr-4'
            }`}
          >
            <div className="mt-0.5">
              {msg.role === 'user' ? (
                <User className="w-3.5 h-3.5 text-indigo-400" />
              ) : (
                <Bot className="w-3.5 h-3.5 text-indigo-500" />
              )}
            </div>
            <div 
              className="flex-1 overflow-x-auto whitespace-pre-line text-main leading-relaxed"
              dangerouslySetInnerHTML={{ __html: msg.content }}
            />
          </div>
        ))}
        {isLoading && (
          <div className="flex items-center space-x-2 p-2 bg-slate-900/40 border border-slate-800/40 rounded mr-4">
            <Bot className="w-3.5 h-3.5 text-indigo-500 animate-spin" />
            <span className="text-muted text-[10px]">正在调动流体算力追踪 C++ 物理镜像消纳，解算自由能梯度...</span>
          </div>
        )}
        <div ref={messagesEndRef} />
      </div>

      {/* Input Form */}
      <form onSubmit={handleSend} className="mt-3 flex items-center space-x-1.5 border-t border-main pt-2">
        <input
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="问问交期延误原因，或者让AI做处方决策..."
          className="flex-1 bg-input border border-main text-heading text-xs px-2.5 py-1.5 rounded focus:outline-none focus:border-indigo-500"
          disabled={isLoading}
        />
        <button
          type="submit"
          className="btn-premium-primary p-2 h-8 w-8 flex items-center justify-center"
          disabled={isLoading}
        >
          <Send className="w-3.5 h-3.5" />
        </button>
      </form>
    </div>
  );
};

export default AiCopilot;
